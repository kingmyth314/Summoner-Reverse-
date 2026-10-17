// unnamed_ai_think_0x4b5140.cpp - Sum.exe v1.40, code 0x4b5140-0x4b77ef: the AI think unit (party
// formation, target scoring and the attack target choice, following a character, the goal handlers, the
// attack-slot spreading of the party's enemies, ai_think). The file name is not known (a placeholder, see
// src/decomp/README.md): no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables and .rdata values read from
// Summoner/Sum.exe). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: the initialiser run 0x4b5140-0x4b5150 (table entry 305, 0x5884c8: an empty initialiser)
// after ai_mode_sneaking_frame (0x4b4eb0, the AI movement unit), to ai_think's RET (0x4b77eb);
// pathfinding_visibility.cpp starts at 0x4b77f0 (sort_points_clockwise, assert path 0x59d7d8).
// Data: no .data; .rdata 0x581bb0 (the team priority table, file-static here) and the float literals
// after it (-4 0x581bf0, 0.0005, 7, 1/3000, 5/(2 pi), 3.2; the others pooled). .bss 0x247e8c8-0x247e963.
//
// Profiling: ai_pick_attack_target, ai_party_follow_think, ai_clear_target_and_idle,
// ai_think_party_update and ai_think open an EH frame whose only unwind action is the empty destructor
// 0x434650: a profile_label scope (vsdk/os/profile.h), the label texts are not in the binary.

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/timer.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/profile.h"
#include "vsdk/math/vector.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/spells/spells.h"
#include "levelscripts/level_scripts.h"
#include "summoner/summoner.h"
#include "summoner/gametarget.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define AIS_TRAVEL_PATH        0x00
#define AIS_TRAVEL_FOLLOW      0x03
#define AIS_TRAVEL_FOLLOW_SLOW 0x04
#define AIS_TRAVEL_WAIT        0x05
#define AIS_TRAVEL_GOTO        0x06
#define AIS_TRAVEL_ESCORT      0x07
#define AIS_TRAVEL_WANDER      0x08
#define AIS_ENTER_HIDING       0x09
#define AIS_ENTER_SNEAKING     0x0c
#define AIS_LEAVE_SNEAKING     0x0d
#define AIS_COMBAT_ENTER       0x0e
#define AIS_COMBAT_MELEE       0x0f
#define AIS_COMBAT_BACKSTAB    0x11
#define AIS_COMBAT_SPREAD      0x22    // walking to a free attack slot (ai_party_spread_attackers)
#define AIS_CAST_SPELL         0x27
#define AIS_CAST_AT_TARGET     0x28
#define AIS_CAST_AT_POINT      0x29
#define AIS_TRAVEL_ASIDE       0x2b

#define AI_FORMATION_LEADER_HANDLE(g) ((g)->leader)

// ---- .rdata ------------------------------------------------------------------------------------------
// The attack goal priority / 10 by [attacker team][target team] (0x581bb0; name ours).
static const int ai_team_attack_priority[4][4] = {
    { 0, 0, 80, 100 },
    { 0, 0, 0, 0 },
    { 100, 60, 10, 25 },
    { 100, 80, 60, 40 },
};

// ---- .bss --------------------------------------------------------------------------------------------
int ai_formation_slot[AI_FORMATION_SLOTS];                    // 0x247e8c8
vector3 ai_formation_member_pos[AI_FORMATION_SLOTS];          // 0x247e8e0
vector3 ai_formation_slot_pos[AI_FORMATION_SLOTS];            // 0x247e920
int ai_formation_count;                                       // 0x247e960

// 0x4b5140 static_init_empty_4b5150 (jmp, static initialiser entry 305) /
// 0x4b5150 init_empty_4b5150 (bare ret). Confidence: unsure (an initialiser that expands to nothing: a
// definition whose constructor is empty, as ai.cpp's init_empty_472000; VC6 /O2 /Ob1 emits exactly this
// pair for a global array of a class with an empty inline constructor such as vector3, D5600. The two
// vector3 arrays above would give two such pairs, the binary has one; which object it is is not known,
// so none is written).

// ---- formation -----------------------------------------------------------------------------------------
// 0x4b5160  Confidence: likely (the group pointer is re-read from the owner and stored back). The leader
// hands its members the formation slots 1..n, each to the nearest free one in member order.
void ai_state::group_assign_formation()
{
    Character *members[AI_FORMATION_SLOTS];
    bool free_slot[AI_FORMATION_SLOTS];
    vector3 slots[AI_FORMATION_SLOTS];
    float best_d = 0.0f;
    int n = 0;
    int i, m;
    Character *c;

    formation_slot = 0;
    group = owner->ai.group;
    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        if (c != owner && c->ai.group == group)
            members[n++] = c;
    for (i = 0; i < AI_FORMATION_SLOTS; i++)
        free_slot[i] = i < n;
    for (i = 0; i < n; i++)
        formation_slot_position(&slots[i], &owner->pos, &owner->orient.fvec, i + 1);
    for (m = 0; m < n; m++) {
        int best = -1;
        for (i = 0; i < n; i++) {
            float d;
            if (!free_slot[i])
                continue;
            d = distance_squared(&members[m]->pos, &slots[i]);
            if (best < 0 || d < best_d) {
                best_d = d;
                best = i;
            }
        }
        free_slot[best] = false;
        members[m]->ai.formation_slot = best + 1;
    }
    ai_formation_count = n;
    for (i = 0; i < n; i++) {
        ai_formation_slot[i] = members[i]->ai.formation_slot - 1;
        ai_formation_member_pos[i] = members[i]->pos;
        ai_formation_slot_pos[i] = slots[i];
    }
}

// 0x4b52e0  Confidence: exact (VC6 byte match) (jump table 0x4b53a4 over slots 1..5; the leader lookup's result is not
// used). Slots: 1 (-2, -2), 2 (2, -2), 3 (0, -4), 4 (4, -4), 5 (-4, -4) as (side, ahead) along dir.
void ai_state::formation_slot_position(vector3 *out, const vector3 *base, const vector3 *dir, int slot)
{
    float side, ahead;

    living_from_handle(owner->ai.group->leader);
    if (slot < 0)
        slot = formation_slot;
    switch (slot) {
    case 1: side = -2.0f; ahead = -2.0f; break;
    case 2: side = 2.0f; ahead = -2.0f; break;
    case 3: side = 0.0f; ahead = -4.0f; break;
    case 4: side = 4.0f; ahead = -4.0f; break;
    case 5: side = -4.0f; ahead = -4.0f; break;
    default: side = 0.0f; ahead = 0.0f; break;
    }
    *out = *base;
    out->x += ahead * dir->x + side * dir->z;
    out->z += ahead * dir->z - side * dir->x;
}

// ---- targets -------------------------------------------------------------------------------------------
// 0x4b53c0  Confidence: likely (two range tests: <= first, then < after the line of sight). A candidate
// passes when it is an awake, visible enemy in range (the range shrinks with a sneaking candidate's hide
// skill), seen in front (dot > 0.71) when beyond the detection range or when sneaking / backstabbing,
// within the height band (7.5 for a boss self, else 3), in sight and not already attacked by
// max_attackers. Its score is the distance, scaled by the previous best's rating over its own and by
// 1.5 - intelligence; a candidate with status 9 scores the range itself.
void ai_score_target(ai_target_query *q, bool boss_list)
{
    Character *c = q->candidate;
    Character *self = q->self;
    float dist, facing, range, rating, score, maxdy;
    vector3 d;
    int style;

    if (c == self || c->type == 4)
        return;
    if (self->type == 9 && !boss_list && species_is_boss_b(self->info->species))
        return;
    if (c->flags & 0x20000)
        return;
    if (!(c->hp > 0.0f))
        return;
    if (c->flags & CF_HIDDEN)
        return;
    if (c->ai.is_mode(AI_STATE_HIDING, -1) == true)
        return;
    if (!self->is_enemy(c))
        return;
    dist = distance(&self->pos, &c->pos);
    d = c->pos - self->pos;
    d.normalize();
    facing = d.z * self->orient.fvec.z + d.y * self->orient.fvec.y + d.x * self->orient.fvec.x;
    range = self->info->fov_range;
    if (c->flags & 0x400) {
        if (c->has_status(8))
            return;
        if (self->has_status(0))
            return;
        range = (1.0f - c->get_skill(0x16) * 0.6f) * range;
    }
    if (dist > range)
        return;
    if (dist > self->info->detection_range && facing < 0.71f)
        return;
    if (self->has_status(0) == true && dist > self->info->detection_range)
        return;
    if (c->has_status(8)) {
        if (c->status_value(8) > 0.0f)
            return;
        if (dist > self->info->detection_range)
            return;
    }
    if ((c->flags & 0x400) || c->ai.is_mode(AI_STATE_COMBAT, AIS_COMBAT_BACKSTAB))
        if (facing < 0.71f)
            return;
    if (cutscene_script_is_active() && c->type == 7)
        return;
    if (c->flags & CF_SUMMON) {
        if (c->ai.is_mode(AI_STATE_SUMMONING, -1))
            return;
        if (!(c->obj_flags & 1))
            return;
    }
    maxdy = self->type == 9 ? 7.5f : 3.0f;
    if (fabsf(c->pos.y - self->pos.y) > maxdy)
        return;
    if (!self->has_line_of_sight_to(c))
        return;
    if (c->ai.count_engaged_attackers(true) >= q->max_attackers)
        return;
    if (!(dist < range))
        return;
    rating = c->combat.attack_rating();
    style = c->ai.get_combat_style();
    if (style == 4 || style == 5 || style == 3)
        rating *= 1.25f;
    score = dist;
    if (q->result) {
        score = (1.5f - self->info->intelligence) * (dist * q->rating / rating);
        if (c->has_status(9))
            score = range;
    }
    if (!q->result || score < q->best) {
        q->best = score;
        q->result = c;
        q->rating = rating;
    }
}

// 0x4b5790  Confidence: likely (MP_IS_CLIENT / MP_IS_HOST as the binary tests them, game_flags & 2
// loaded once). Not for bosses; the Luminar keeps its target until luminar_can_switch_target. A locally
// controlled, idle (or waiting) character hit by an enemy turns on it unless its current target is
// nearer by the intelligence factor.
void ai_state::retaliate_on_hit(int attacker)
{
    Character *a;

    if (owner->type == 9) {
        if (species_is_boss_b(owner->info->species))
            return;
        if (owner->info->species == SPECIES_LUMINAR && target != -1 && living_from_handle(target)
            && !luminar_can_switch_target())
            return;
    }
    if (MP_IS_CLIENT())
        return;
    if (MP_IS_HOST() && !character_is_locally_controlled(owner))
        return;
    if (flags & 0x10)
        return;
    if ((owner->ai.flags & AI_FLAG_PLAYER_CONTROL) && !is_mode(AI_STATE_IDLE, -1)
        && !is_mode(AI_STATE_TRAVELING, AIS_TRAVEL_WAIT))
        return;
    if (attacker == target)
        return;
    a = living_from_handle(attacker);
    if (!a)
        return;
    if (!owner->is_enemy(a))
        return;
    if (target != -1) {
        float da = distance_squared(&owner->pos, &a->pos);
        Character *t = living_from_handle(target);
        if (t) {
            float dt = distance_squared(&owner->pos, &t->pos);
            if (owner->info->intelligence * 4.0f * da >= dt)
                return;
        }
    }
    add_attack(a, 1000);
}

// 0x4b5900  Confidence: likely. With a weapon skill: a live target in reach (1.5 * attack radius + 2) or
// any target of a monster (type 9) is kept; otherwise every living entity is scored and an attack goal
// is added on the best one, at the team table's priority.
void ai_state::pick_attack_target(int max_attackers)
{
    profile_label label("ai_pick_attack_target");
    Character *t = NULL;
    Character *best;
    int cur;
    ai_target_query q;

    if (!owner->combat.has_weapon_skill())
        return;
    if (target != -1) {
        t = living_from_handle(target);
        if (!t || (t->flags & CF_DEAD))
            target = -1;
    }
    cur = target;
    if (cur != -1) {
        if (distance(&owner->pos, &t->pos) < owner->info->attack_radius * 1.5f + 2.0f)
            return;
        if (owner->type == 9)
            return;
        target = -1;
    }
    q.self = owner;
    q.max_attackers = max_attackers;
    q.best = owner->info->fov_range;
    q.result = NULL;
    q.rating = 0.0f;
    if (target == -1) {
        Character *c;
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            q.candidate = c;
            ai_score_target(&q, false);
        }
    }
    best = q.result;
    if ((!best || best == living_from_handle(cur)) && cur != -1) {
        target = cur;
        return;
    }
    if (best && best->handle != cur)
        add_attack(best, ai_team_attack_priority[owner->get_team()][best->get_team()] * 10);
}

// ---- following -----------------------------------------------------------------------------------------
// 0x4b5a80  Confidence: likely. While already travelling (sub-mode 3) to within 0.3 of the target, keep
// going; else path to where the target will be (its remaining think time ahead), stopping the sizes and
// 2 units short.
int ai_state::follow_repath(Character *self, Character *target)
{
    vector3 dest;
    vector3 p;
    int layer;
    float stop;

    if (mode == AI_STATE_TRAVELING && submode == AIS_TRAVEL_FOLLOW && path_get_destination(&dest)
        && distance_squared(&dest, &target->pos) < 0.1f)
        return 0;
    target->ai.predict_owner_position(&p, timer3c.GetRemainingTime() * 0.0005f);
    layer = pf_trace_edge_crossings(&target->pos, target->layer, p);
    stop = self->size_radius() + target->size_radius() + 2.0f;
    path_find_new(&self->pos, self->layer, &p, layer, self->info->size, stop);
    if (!path)
        return -1;
    if ((game_flags & 2) && character_is_locally_controlled(self))
        mp_send_character_walk(self, 0);
    set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_FOLLOW);
    return 0;
}

// 0x4b5bd0  Confidence: exact (VC6 byte match). A member of the target's group (the target its leader) walks to its
// formation slot around where the leader will be; close (within 25 units squared for a party leader, 9
// otherwise) it waits (sub-mode 5), ahead of a moving leader it lets it pass (sub-mode 4). Anyone else
// follows by edge distance: beyond 100 it gives up (1), beyond 3 it re-paths (ai_follow_repath).
int ai_state::follow_character(Character *t)
{
    float near_d2;
    vector3 p;
    vector3 slot;
    int layer;
    float len;
    float e;

    if (!t || (t->flags & CF_HIDDEN))
        return -1;
    near_d2 = 25.0f;
    if (t->type != 7)
        near_d2 = 9.0f;
    if (group && group == t->ai.group && group->leader == t->handle) {
        ai_path *pp;
        float dt = timer3c.GetRemainingTime() * (1.0f / 3000.0f);

        t->ai.predict_owner_position(&p, dt);
        if ((!is_mode(AI_STATE_TRAVELING, AIS_TRAVEL_FOLLOW) && !is_mode(AI_STATE_TRAVELING, AIS_TRAVEL_FOLLOW_SLOW))
            || t->get_698() < 0.01f) {
            p.y = owner->pos.y;
            if (distance_squared(&p, &owner->pos) < near_d2) {
                ai_path_free(path);
                path = NULL;
                set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_WAIT);
                return 0;
            }
        }
        vector3 *fwd = &t->orient.fvec;
        owner->ai.formation_slot_position(&slot, &p, fwd, -1);
        slot.y = owner->pos.y;
        pf_find_nearest_navigable_point(&slot, owner->info->size, owner->layer);
        layer = pf_trace_edge_crossings(&t->pos, t->layer, slot);
        pp = ai_path_find(&owner->pos, owner->layer, &slot, layer, owner->info->size, 0.0f);
        if (pp)
            len = distance(&pp->pos, &slot) + pp->cost;
        else
            len = 7.0f;
        ai_path_free(pp);
        if (t == cur_character && len < 20.0f)
            flags |= 0x8000000;
        if (len < 7.0f) {
            float d2 = distance_squared(&slot, &owner->pos);
            float thr;
            if (t->get_698() > 0.0f) {
                vector3 to;
                to = slot - owner->pos;
                to.normalize();
                if (to.x * fwd->x + to.y * fwd->y + to.z * fwd->z < 0.0f) {
                    ai_path_free(path);
                    path = NULL;
                    set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_FOLLOW_SLOW);
                    return 0;
                }
            }
            thr = 0.25f;
            if (t->forward_speed < 0.1f)
                thr = 16.0f;
            if (d2 > thr) {
                layer = pf_trace_edge_crossings(&t->pos, t->layer, slot);
                path_find_new(&owner->pos, owner->layer, &slot, layer, owner->info->size, 0.0f);
                if (!path)
                    return 1;
                if ((game_flags & 2) && character_is_locally_controlled(owner))
                    mp_send_character_walk(owner, 0);
            }
            set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_FOLLOW);
            return 0;
        }
    }
    e = owner->distance_between_edges(t, false, -1);
    if (e > 100.0f)
        return 1;
    if (e > 3.0f && follow_repath(owner, t))
        return 1;
    return 0;
}

// ---- goal handlers ---------------------------------------------------------------------------------------
// 0x4b6000  Confidence: exact (VC6 byte match). Busy targets (all five attack slots taken by others) and invisible ones
// (status 8) are left alone; the travel goals go, the combat target is set; a party member out of the
// ready stance asks for combat mode first.
int ai_state::goal_do_attack(int target_handle, unsigned int goal_flags)
{
    Character *t;
    float d;

    if (!owner->combat.has_weapon_skill())
        return -1;
    if (owner->type == 4)
        return -1;
    t = living_from_handle(target_handle);
    if (!t || (t->flags & CF_HIDDEN))
        return -1;
    d = owner->distance_between_edges(t, false, -1);
    if (character_target_point_count(t) > 0 && t->ai.count_attack_slots_used() == 5) {
        int i;
        for (i = 0; i < 5; i++)
            if (t->ai.attackers[i].handle == owner->handle)
                break;
        if (i == 5)
            return 1;
    }
    if (t->has_status(8)) {
        if (t->status_value(8) > 0.0f)
            return 1;
        if (d > owner->info->detection_range)
            return 1;
    }
    while (1) {
        if (!goal_remove(goal_find(1, AI_GOAL_PATHFIND, 0, 0, -1, 0, 0, AI_NO_SPELL)))
            break;
    }
    if (target == target_handle && mode == AI_STATE_COMBAT)
        return 0;
    target = target_handle;
    _unk7c = -1;
    owner->combat.set_target((int)owner, t);
    if (goal_flags & 2)
        call_for_help();
    if (!(owner->flags & CF_COMBAT_READY)) {
        if (owner->type == 7) {
            if (combat_mode && !is_combat_e_or_sneak_attack()) {
                set_combat_mode_goal(true, 999999);
                process_goals();
            }
            return 0;
        }
        go_idle();
        set_combat_mode_goal(true, 999999);
        return 0;
    }
    if ((goal_flags & 0x100) || t->has_status(9))
        flags |= 0x10;
    if (!combat_approach(t) && !is_mode(AI_STATE_COMBAT, -1))
        set_mode(AI_STATE_COMBAT, AIS_COMBAT_MELEE);
    if ((game_flags & 2) && character_is_locally_controlled(owner))
        mp_send_character_attack(owner);
    path_step = 0;
    return 0;
}

// 0x4b6240  Confidence: exact. A party member told to follow the leader it is already grouped under
// does nothing.
int ai_state::goal_do_follow(int target_handle)
{
    Character *t = living_from_handle(target_handle);

    if (!t)
        return -1;
    if (owner->type == 7 && t == cur_character && (!group || group->leader != cur_character->handle))
        return 1;
    return follow_character(t);
}

// 0x4b62a0  Confidence: likely. A rider moves its mount; the walk flag (goal flag 0x10) goes to the
// other machines, goal flag 0x200 clears path_to's keep flag. The priority is not read.
int ai_state::goal_do_travel(vector3 *pos, int layer, int priority, unsigned int goal_flags)
{
    Character *mount = owner->get_mount();
    int r;

    if (mount) {
        ai_state *m = &mount->ai;
        r = m->path_to(&mount->pos, mount->layer, pos, layer, 0, mount->info->size, false, true);
        if (r == 0) {
            m->set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_PATH);
            set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_PATH);
            if ((game_flags & 2) && character_is_locally_controlled(owner))
                mp_send_character_walk(owner, (goal_flags >> 4) & 1);
        }
        return r;
    }
    if (owner->type == 7)
        owner->instance->clear_action_anims();
    r = path_to(&owner->pos, owner->layer, pos, layer, 0, owner->info->size, false,
                (bool)(~(goal_flags >> 9) & 1));
    if (r == 0) {
        set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_PATH);
        if ((game_flags & 2) && character_is_locally_controlled(owner))
            mp_send_character_walk(owner, (goal_flags >> 4) & 1);
    }
    return r;
}

// 0x4b63e0  Confidence: exact.
int ai_state::goal_do_nothing()
{
    return -1;
}

// 0x4b63f0  Confidence: exact. Only while the owner is in the ready stance.
int ai_state::goal_do_approach_party(int target_handle)
{
    Character *t = living_from_handle(target_handle);

    if (!t)
        return -1;
    if (!(owner->flags & CF_COMBAT_READY))
        return -1;
    return follow_character(t);
}

// 0x4b6430  Confidence: exact (VC6 byte match) (the store order of the reset).
int ai_state::goal_do_combat_mode()
{
    if (owner->type == 7) {
        if (is_combat_e_or_sneak_attack() == true)
            return 1;
        if (owner->flags & 0x400) {
            add_sneak();
            process_goals();
            return 1;
        }
    } else if (is_combat_e_or_sneak_attack() == true) {
        return 0;
    }
    owner->set_6ac(0.0f);
    emerge_move = -1;
    flags &= 0x7fffffff;
    disable_timer.reset();
    set_mode(AI_STATE_COMBAT, AIS_COMBAT_ENTER);
    return 0;
}

// 0x4b64c0  Confidence: exact.
int ai_state::goal_do_cast_spell(int spell)
{
    flags &= ~0x40000000;
    set_mode(AI_STATE_CAST_SPELL, AIS_CAST_SPELL);
    mode_target = spell;
    emerge_move = -1;
    owner->set_6ac(0.0f);
    return 0;
}

// 0x4b6500  Confidence: exact.
int ai_state::goal_do_cast_at_target(int spell, int target_handle)
{
    if (!living_from_handle(target_handle))
        return -1;
    target = target_handle;
    flags &= ~0x40000000;
    set_mode(AI_STATE_CAST_SPELL, AIS_CAST_AT_TARGET);
    mode_target = spell;
    emerge_move = -1;
    owner->set_6ac(0.0f);
    return 0;
}

// 0x4b6560  Confidence: exact.
int ai_state::goal_do_cast_at_point(int spell, vector3 *pos, int layer)
{
    cast_point = *pos;
    cast_arg = layer;
    flags &= ~0x40000000;
    set_mode(AI_STATE_CAST_SPELL, AIS_CAST_AT_POINT);
    mode_target = spell;
    emerge_move = -1;
    owner->set_6ac(0.0f);
    return 0;
}

// 0x4b65c0  Confidence: exact.
int ai_state::goal_do_move_to(vector3 *pos, int layer, bool walk, ai_path *p)
{
    if (!p)
        path_to(&owner->pos, owner->layer, pos, layer, 0, owner->info->size, false, true);
    else
        set_path(&owner->pos, p);
    path_timer_b.SetTimeout(1000);
    path_timer_a.SetTimeout(1000);
    set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_GOTO);
    if (walk)
        owner->set_speed_scale(owner->walk_speed, false);
    else
        owner->set_speed_scale(owner->run_speed, false);
    if ((game_flags & 2) && character_is_locally_controlled(owner))
        mp_send_character_walk(owner, walk);
    return 0;
}

// 0x4b6690  Confidence: exact. The wander centre and radius for sub-mode 8.
void ai_state::goal_do_move_to_timed(vector3 *pos, int layer, float arg, bool walk)
{
    cast_point = *pos;
    cast_arg = layer;
    wander_radius = arg;
    set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_WANDER);
    if (walk)
        owner->set_speed_scale(owner->walk_speed, false);
    else
        owner->set_speed_scale(owner->run_speed, false);
    timer40.SetTimeout(0);
}

// 0x4b6710  Confidence: exact.
void ai_state::goal_do_hide()
{
    if (!(owner->flags & CF_COMBAT_READY)) {
        if (owner->type == 7) {
            combat_mode_set(true);
            return;
        }
        owner->set_6ac(0.0f);
        set_combat_mode_goal(true, 999999);
        return;
    }
    if (owner->flags & 0x400) {
        add_sneak();
        return;
    }
    if (!is_mode(AI_STATE_HIDING, -1)) {
        flags |= 2;
        set_mode(AI_STATE_HIDING, AIS_ENTER_HIDING);
        if (game_flags & 2)
            mp_send_swi_hide(owner);
    }
}

// 0x4b6790  Confidence: exact.
void ai_state::goal_do_sneak()
{
    if (!(owner->flags & CF_COMBAT_READY)) {
        if (owner->type == 7) {
            combat_mode_set(true);
            return;
        }
        owner->set_6ac(0.0f);
        set_combat_mode_goal(true, 999999);
        return;
    }
    disable_timer.invalidate();
    if (owner->flags & 0x400) {
        timer44.invalidate();
        set_mode(AI_STATE_SNEAKING, AIS_LEAVE_SNEAKING);
    } else {
        set_mode(AI_STATE_SNEAKING, AIS_ENTER_SNEAKING);
    }
    if (game_flags & 2)
        mp_send_swi_sneak(owner);
}

// 0x4b6810  Confidence: exact (VC6 byte match) (store order). The backstab within 2 units edge distance.
void ai_state::goal_do_special_attack(int target_handle)
{
    Character *t = living_from_handle(target_handle);

    if (!t || (t->flags & CF_HIDDEN))
        return;
    if (owner->distance_between_edges(t, false, -1) > 2.0f)
        return;
    if (owner->flags & 0x400) {
        add_sneak();
        return;
    }
    target = target_handle;
    flags |= 0x10;
    _unk7c = -1;
    owner->combat.target = -1;
    owner->combat.set_target((int)owner, t);
    owner->combat.set_38(SKILL_BACKSTAB);
    set_mode(AI_STATE_COMBAT, AIS_COMBAT_BACKSTAB);
}

// 0x4b68c0  Confidence: exact. Items (type 3) and gold (type 0xb) within 1.5 units.
void ai_state::goal_do_pickup(int target_handle)
{
    obj *o;

    goal_remove(0);
    o = obj_from_handle(target_handle);
    if (o && (o->type == 3 || o->type == 0xb)) {
        if ((o->pos - owner->pos).len() <= 2.25f)
            item_obj_pickup((item_obj *)o, 1);
    }
}

// ---- think helpers -------------------------------------------------------------------------------------
// 0x4b6940  Confidence: exact. Follows the target of the follow goal (or, in the ready stance, of the
// protect goal); a type-4 owner matches the target's speed.
void ai_state::party_follow_think()
{
    profile_label label("ai_party_follow_think");
    int g = goal_find(1, AI_GOAL_FOLLOW, 0, 0, -1, 0, 0, AI_NO_SPELL);
    Character *t;

    if (g < 0 && (owner->flags & CF_COMBAT_READY))
        g = goal_find(1, AI_GOAL_PROTECT, 0, 0, -1, 0, 0, AI_NO_SPELL);
    if (g < 0)
        goto idle;
    t = living_from_handle(goals[g].target);
    if (owner->type == 7 && t == cur_character) {
        if (!group || group->leader != cur_character->handle)
            goto idle;
    } else if (owner->type == 4) {
        if (!t)
            return;
        owner->set_speed_scale(t->speed_scale, false);
    }
    if (!t)
        return;
    if (follow_character(t) >= 0)
        return;
idle:
    go_idle();
}

// 0x4b6a30  Confidence: exact.
void ai_state::clear_target_and_idle()
{
    profile_label label("ai_clear_target_and_idle");

    priority = -1;
    go_idle();
}

// 0x4b6a70  Confidence: exact. qsort comparator of the {handle, angle} pairs of ai_party_spread_attackers.
static int qsort_cmp_float_field4(const void *a, const void *b)
{
    if (((const float *)a)[1] < ((const float *)b)[1])
        return -1;
    return 1;
}

// 0x4b6a90  Confidence: likely. The two slot directions nearest the attacker's side; the result is the
// best slot plus the fraction toward the second (asin of the cross product, 5 / (2 pi) slots per radian),
// wrapped into 0..5.
float ai_choose_attack_slot(Character *attacker, Character *target)
{
    int best = -1, second = -1;
    float best_dot = -999.9f, second_dot = -999.9f;
    vector3 tc = target->get_center();
    vector3 d = (attacker->pos + attacker->centre_offset) - tc;
    vector3 sd;
    int i, sign;
    float s, r;

    d.normalize_safe();
    for (i = 0; i < 5; i++) {
        float dot;
        target->ai.attack_slot_direction(&sd, i);
        dot = sd.z * d.z + sd.y * d.y + sd.x * d.x;
        if (dot > best_dot) {
            second_dot = best_dot;
            second = best;
            best_dot = dot;
            best = i;
        } else if (dot > second_dot) {
            second_dot = dot;
            second = i;
        }
    }
    sign = second - best > 0 ? 1 : -1;
    target->ai.attack_slot_direction(&sd, best);
    s = (float)fabs(sd.z * d.x - sd.x * d.z);
    r = (float)asin(s) * 0.7957747f * sign + best;
    if (r < 0.0f)
        r += 5.0f;
    return r;
}

// A GameTimer with an empty destructor (name ours, D4806): ai_party_spread_attackers' function-static
// timer registers an atexit destructor (0x4b71b0, a bare RET), which GameTimer itself does not have (as
// unnamed_camera_0x4778b0.cpp's cam_vec, D3805).
class spread_timer : public GameTimer {
public:
    ~spread_timer() {}
};
// 0x4b71b0 atexit_nop_4b71b0: the atexit destructor of the timer below (~spread_timer). Confidence: likely.

// 0x4b6c10  Confidence: unsure (the scan's index arithmetic is the binary's; the jump into one exit
// block is written with goto). Every 2 s: for each enemy attacking the player's group (not one with
// attack points), its attackers are sorted by the slot they take around it; one that crowds its
// neighbours (closer than 0.75 slot) walks to a free slot beside them (sub-mode 0x22). A member already
// doing so stops the whole pass.
void ai_state::party_spread_attackers()
{
    static spread_timer timer;
    struct { int handle; float slot; } list[6];
    int targets[6];
    ai_group *g;
    int nt, i, k;

    if (!timer.elapsed() && timer.valid())
        return;
    timer.SetTimeout(2000);
    g = cur_character->ai.group;
    if (!g)
        return;
    nt = 0;
    for (i = 0; i < g->count; i++) {
        Character *m = living_from_handle(g->members[i]);
        Character *t;
        int j;
        if (!m)
            continue;
        if (m->ai.submode == AIS_COMBAT_SPREAD)
            return;
        if (!m->ai.is_attacking())
            continue;
        for (j = 0; j < nt; j++)
            if (targets[j] == m->ai.target)
                break;
        if (j < nt)
            continue;
        t = living_from_handle(m->ai.target);
        if (!t || t->info->num_target_points != 0)
            continue;
        targets[nt++] = m->ai.target;
    }
    for (k = 0; k < nt; k++) {
        int n = 0;
        int pick;
        float slot;

        for (i = 0; i < g->count; i++) {
            Character *m = living_from_handle(g->members[i]);
            if (!m || !m->ai.is_attacking() || m->ai.target != targets[k])
                continue;
            list[n].handle = m->handle;
            list[n].slot = ai_choose_attack_slot(m, living_from_handle(m->ai.target));
            n++;
        }
        if (n < 1)
            continue;
        qsort(list, n, 8, qsort_cmp_float_field4);
        if (n == 2) {
            if (!(list[1].slot - list[0].slot < 0.75f))
                continue;
            slot = list[0].slot + 1.0f;
            pick = 1;
            goto move;
        }
        if (n < 2)
            continue;
        {
            float prev = list[n - 1].slot;
            for (i = 0; i < n; i++) {
                float gap = list[i].slot - prev;
                if (gap < 0.0f)
                    gap += 5.0f;
                if (gap < 0.75f) {
                    int j, jj;
                    float gap2 = list[(i + 1) % n].slot - list[i].slot;
                    if (gap2 < 0.0f)
                        gap2 += 5.0f;
                    if (gap2 >= 2.0f) {
                        slot = list[i].slot + 1.0f;
                        pick = i;
                        goto move;
                    }
                    if (gap2 >= 1.5f) {
                        slot = list[i].slot + 0.75f;
                        pick = i;
                        goto move;
                    }
                    j = i >= 1 ? i - 1 : i + n - 1;
                    jj = i - 2 >= 0 ? i - 2 : i + n - 2;
                    gap2 = list[j].slot - list[jj].slot;
                    if (gap2 < 0.0f)
                        gap2 += 5.0f;
                    if (gap2 >= 2.0f) {
                        slot = list[j].slot - 1.0f;
                        pick = j;
                        goto move;
                    }
                    if (gap2 >= 1.5f) {
                        slot = list[j].slot - 0.75f;
                        pick = j;
                        goto move;
                    }
                }
                prev = list[i].slot;
            }
        }
        continue;

    move:
        {
            int s0, s1;
            float frac;
            Character *m, *t;
            vector3 sd, dir, dest;
            float r;

            if (slot < 0.0f)
                slot += 5.0f;
            s0 = (int)floor(slot);
            s1 = s0 + 1;
            if (s1 == 5)
                s1 = 0;
            frac = slot - s0;
            m = living_from_handle(list[pick].handle);
            if (!m)
                continue;
            t = living_from_handle(m->ai.target);
            if (!t)
                continue;
            t->ai.attack_slot_direction(&sd, s0);
            dir = sd * (1.0f - frac);
            t->ai.attack_slot_direction(&sd, s1);
            dir += sd * frac;
            dir.normalize_safe();
            r = (t->size_radius() + m->size_radius() + m->ai.attack_range) * 0.66f;
            dest = t->get_center() + dir * r;
            if (!pf_point_is_navigable(&dest, m->info->size, m->layer))
                continue;
            if (!m->ai.path_find_new(&m->pos, m->layer, &dest, m->layer, m->info->size, 0.0f))
                continue;
            m->ai.set_mode(AI_STATE_COMBAT, AIS_COMBAT_SPREAD);
        }
    }
}

// 0x4b71c0  Confidence: likely (the GameTimer of living_combat +0xc is an int in combat.h, D300). Not for
// types 1 and 4: the leader assigns the formation; stale attacker slots and attack points are freed; a
// character that hit the owner in the last 200 ms from behind within 3.2 units makes it step aside;
// neighbours within 2.5 claim slots on each other. A party member also follows the party leader (or drops
// the follow goal when it is the leader) and a summon too far from Joseph is dismissed.
void ai_state::think_party_update()
{
    profile_label label("ai_think_party_update");
    Character *c;
    int i;

    if (owner->type == 4 || owner->type == 1)
        return;
    if (group && group->leader == owner->handle)
        group_assign_formation();
    for (i = 0; i < 5; i++) {
        Character *a = living_from_handle(attackers[i].handle);
        if (a && distance(&a->pos, &owner->pos) > 2.5f
            && (a->ai.is_combat_sub16() != true || a->ai.target != owner->handle))
            attackers[i].handle = -1;
    }
    for (i = 0; i < 8; i++) {
        Character *a = living_from_handle(attack_point_users[i]);
        if (a && (a->ai.target != owner->handle || a->ai._unk7c != i))
            attack_point_users[i] = -1;
    }
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        GameTimer *hit_timer;
        if (c == owner)
            continue;
        hit_timer = (GameTimer *)&c->combat.timer;
        if (hit_timer->elapsed() && hit_timer->GetElapsedTime() < 200) {
            vector3 d = owner->pos - c->pos;
            if (d.normalize_length() < 3.2f
                && d.z * c->orient.fvec.z + d.y * c->orient.fvec.y + d.x * c->orient.fvec.x < -0.5f) {
                vector3 dest;
                if (d.z * c->orient.rvec.z + d.y * c->orient.rvec.y + d.x * c->orient.rvec.x > 0.0f)
                    dest = owner->pos + c->orient.rvec * 2.5f;
                else
                    dest = owner->pos - c->orient.rvec * 2.5f;
                owner->ai_add_goal9(&dest, owner->layer, 5000, true);
                owner->ai.process_goals();
            }
        }
        if (distance(&c->pos, &owner->pos) <= 2.5f) {
            claim_attack_slot(owner, c, 0, -1);
            c->ai.claim_attack_slot(c, owner, 0, -1);
        }
    }
    if (owner->type == 7) {
        party_spread_attackers();
        if (owner == cur_character)
            goal_remove(goal_find(1, AI_GOAL_FOLLOW, 0, 0, -1, 0, 0, AI_NO_SPELL));
        else
            goal_add(AI_GOAL_FOLLOW, 500, 1, party_leader->handle, NULL, 0, AI_NO_SPELL, 0.0f);
        if (!(game_flags & 0x1000) && (owner->flags & CF_SUMMON)
            && (!(game_flags & 2) || player_from_handle(owner->handle)->mp_owner == mp_local_player)) {
            for (i = 0; i < group->count; i++) {
                Character *m = living_from_handle(group->members[i]);
                if (m->info->species == 0 && owner->distance_between_edges(m, false, -1) > 50.0f) {
                    if (game_flags & 2)
                        mp_send_summon_quick(owner->handle);
                    summon_dismiss(owner->handle);
                    break;
                }
            }
        }
    }
    if (!(flags & AI_FLAG_PLAYER_CONTROL) && !group)
        pick_attack_target(2);
}

// The per-mode think hooks of ai_think: empty, folded into empty_function (0x4f13e0). Confidence: unsure
// (that they are four functions and their names are ours).
void ai_state::think_idle() {}
void ai_state::think_travel_wait() {}
void ai_state::think_travel_escort() {}
void ai_state::think_combat() {}

// 0x4b7610  Confidence: exact (VC6 byte match, match30; was likely; D8902: the travel sub-mode as an if-chain, t_party declared first). The think interval: a party member 500 ms while following the leader,
// else 50-100 ms; any other character 50 ms in combat, else 250-500 ms. A multiplayer client thinks only
// for its own characters' goals.
void ai_state::think()
{
    profile_label label("ai_think");
    int t_party = 0;
    int t_goals = 0;
    int t0, t1;

    if (owner->flags & 0xa)
        return;
    if (!timer3c.elapsed())
        return;
    flags &= ~0x8000000;
    t0 = timer_get(1000000);
    if (owner->type == 7) {
        if (is_following_leader() == true)
            timer3c.SetTimeout(500);
        else
            timer3c.SetRandomTimeout(50, 100);
    } else if (mode == AI_STATE_COMBAT) {
        timer3c.SetTimeout(50);
    } else {
        timer3c.SetRandomTimeout(250, 500);
    }
    if (!MP_IS_CLIENT() || character_is_locally_controlled(owner)) {
        t_party = timer_get(1000000);
        think_party_update();
        process_goals();
        t_goals = timer_get(1000000);
    }
    switch (mode) {
    case AI_STATE_IDLE:
        think_idle();
        break;
    case AI_STATE_TRAVELING:
        // D8902: an if-chain (the binary compares 0, 3 / 5 / 4, 6 / 7, 0x2b in turn; a switch is a table).
        if (submode == AIS_TRAVEL_PATH)
            think_travel_wait();
        else if (submode == AIS_TRAVEL_FOLLOW || submode == AIS_TRAVEL_WAIT || submode == AIS_TRAVEL_FOLLOW_SLOW)
            party_follow_think();
        else if (submode == AIS_TRAVEL_GOTO || submode == AIS_TRAVEL_ESCORT)
            think_travel_escort();
        else if (submode == AIS_TRAVEL_ASIDE)
            clear_target_and_idle();
        break;
    case AI_STATE_COMBAT:
        think_combat();
        break;
    }
    t1 = timer_get(1000000);
    ai_prof_think_a_us += t1 - t0;
    ai_prof_think_c_us += t_goals - t_party;
    if (owner->handle == world_mouse_target_handle) {
        ai_prof_think_b_us += t1 - t0;
        ai_prof_think_d_us += t_goals - t_party;
    }
}
