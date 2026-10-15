// unnamed_ai_combat_0x4bce60.cpp - Sum.exe v1.40, code 0x4bce60-0x4c0245: the AI's combat mode (approach,
// attack slots and points, melee / ranged / close strikes, the ready stance, spell choice for casters and
// healers) and its frame dispatcher ai_mode_combat_frame. File name NOT known: a placeholder
// (src/decomp/README.md). No assert path, no strings, no strings.tbl block; cut by the initialiser
// entry 316 (0x5884f4, an empty one) and the data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact = expect identical code, likely =
// same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_empty_4bce70 (0x4bce60) to ai_mode_combat_frame (0x4bff60-0x4c0245, its jump
// tables end at 0x4c0245). The projector unit (0x4c0250) has no initialiser and no data of its own here.
// Data: no .data, no strings; .bss ai_combat_aim_point 0x2483558-0x2483563 (after the decal unit's
// decal_age 0x2483554). It writes two globals of the AI movement unit (0x247e878, 0x247e84c) and reads
// spells[] (0x22e4f28), spell_global_timers (0x941514) and titus_turn_lock (0x248b914).
// The member functions are thiscall on ai_state (sum.symbols ai_*). ai_state +0x50 flags used here: 2 in a
// stance change, 4 no spells, 0x10 target changed, 0x20 attack points, 0x80 no spell this approach,
// 0x100 going to the ready stance, 0x10000000 approach requested, 0x80000000 stance flipped.

#include <math.h>
#include <stdlib.h>
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "summoner/summoner.h"
#include "summoner/gametarget.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define AIS_TRANSITION      0x0e      // as in ai.cpp (sub-mode names 0x59ad94; D1906 for the shifted ones)
#define AIS_CLOSE_COMBAT    0x0f
#define AIS_RANGED_COMBAT   0x10
#define AIS_BACKSTAB        0x11
#define AIS_CHOOSE_SPELL    0x15
#define AIS_GET_CLOSE       0x16
#define AIS_GET_FAR         0x17
#define AIS_22              0x22      // no safe table name (D1906 shift starts here); follow the path, re-decide after 2.5 s
#define AIS_CHARGE          0x3f

#define CHAR_TYPE_PARTY 7             // obj +0xc
#define CHAR_TYPE_BOSS  9

#define SPELL_HEAL          0x00
#define SPELL_CURE          0x01
#define SPELL_VITALIZE      0x04
#define SPELL_POISON        0x06
#define SPELL_BLIND         0x07
#define SPELL_SILENCE       0x08
#define SPELL_DEATH         0x0c
#define SPELL_LIFE_DRAIN    0x0d
#define SPELL_LIGHTNING     0x0f
#define SPELL_POWER_LEECH   0x11
#define SPELL_JADE_BEAM     0x18
#define SPELL_METEOR_STORM  0x1c
#define SPELL_INFERNO       0x1e
#define SPELL_ICICLE        0x20
#define SPELL_ICY_SLEEP     0x21
#define SPELL_FREEZE        0x22
#define SPELL_ICE_COFFIN    0x24
#define SPELL_BLUE_IMP_SLEEP 0x25

#define SPECIES_POISON_ELEMENTAL 0xca // names ours, from the spell each species gets here
#define SPECIES_JADE_GOLEM       0x09
#define SPECIES_LIFE_DRAINER     0xb9

// Record flags (boss_target +0x0c) set by ai_choose_offensive_spell and the consider functions.
#define AIT_HURT        0x001         // support: below half HP
#define AIT_CURABLE     0x002         // support: a curable status
#define AIT_BLIND_OK    0x010
#define AIT_SILENCE_OK  0x020
#define AIT_ASLEEP      0x040
#define AIT_ICE_COFFIN  0x080
#define AIT_PARALYZED   0x100
#define AIT_BLIND       0x200
#define AIT_SILENCED    0x400
#define AIT_POISONED    0x800

// ---- .bss ---------------------------------------------------------------------------------------------
vector3 ai_combat_aim_point;          // 0x2483558

// 0x4bce60  Confidence: exact. static_init_empty_4bce70: "jmp 0x4bce70" (initialiser entry 316).
// 0x4bce70  Confidence: likely. init_empty_4bce70: a bare ret, the compiler's initialiser for
// ai_combat_aim_point (vector3() is empty; the call to it is gone, the registration stays).

// 0x4bce80  Confidence: exact. sum.symbols calls it ai_state__within_attack_radius, but ECX is the AI
// record: +0x34 is ai_state::attack_range (D3603).
bool ai_state::within_attack_radius(float dist)
{
    return dist < attack_range + 0.2f;
}

// 0x4bcea0  Confidence: likely (the temporaries). A path to a point 0.5-1 attack ranges short of the
// target, on the line from the owner; the mount travels; multiplayer tells the others.
bool ai_state::approach_random_offset(Character *target)
{
    float r = frand_range(0.5f, 1.0f) * attack_range;
    vector3 dest = target->pos;
    vector3 dir;
    Character *mount;

    dir.x = target->pos.x - owner->pos.x;
    dir.y = 0.0f;
    dir.z = target->pos.z - owner->pos.z;
    dir.normalize();
    dest.x -= dir.x * r;
    dest.y -= dir.y * r;
    dest.z -= dir.z * r;
    if (path_find_new(&owner->pos, owner->layer, &dest, target->layer, owner->info->size, 0.0f) == NULL)
        return false;
    mount = owner->get_mount();
    if (mount)
        mount->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
    if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(owner))
        mp_send_character_walk(owner, 0);
    return true;
}

// 0x4bcff0  Confidence: likely. Picks the nearest free attack point of the target (bosses: only the ones
// their stage enables) or keeps the one held (_unk7c), paths there (to the owner's layer when the
// target's fails) and takes the point. With no free point the index stays -1 and the store below writes
// attack_point_users[-1], the fifth attacker slot's timer (NOTES.md, game bugs).
bool ai_state::approach_target_point(Character *target)
{
    vector3 dest;
    vector3 bone_pos;
    vector3 p;
    matrix bone_orient;
    float best;
    float d;
    int best_index;
    int i;

    if (_unk7c == -1) {
        best_index = -1;
        dest = target->pos;
        best = -1.0f;
        for (i = 0; i < target->info->num_target_points; i++) {
            if (target->ai.attack_point_users[i] != -1)
                continue;
            if (target->type == CHAR_TYPE_BOSS && !boss_stage_attack_enabled_dispatch(target->info->species, i))
                continue;
            target->instance->get_bone_world_transform(target->info->target_points[i], &target->orient,
                                                       &target->pos, &bone_orient, &bone_pos);
            p.x = bone_pos.x + target->centre_offset.x;
            p.y = bone_pos.y + target->centre_offset.y;
            p.z = bone_pos.z + target->centre_offset.z;
            d = distance(&p, &owner->pos);
            if (best < 0.0f || d < best) {
                best_index = i;
                best = d;
                dest = p;
            }
        }
        _unk7c = best_index;
    } else {
        target->instance->get_bone_world_transform(target->info->target_points[_unk7c], &target->orient,
                                                   &target->pos, &bone_orient, &bone_pos);
        dest.x = bone_pos.x + target->centre_offset.x;
        dest.y = bone_pos.y + target->centre_offset.y;
        dest.z = bone_pos.z + target->centre_offset.z;
    }
    path_find_new(&owner->pos, owner->layer, &dest, target->layer, owner->info->size, 0.0f);
    if (path == NULL) {
        path_find_new(&owner->pos, owner->layer, &dest, owner->layer, owner->info->size, 0.0f);
        if (path == NULL)
            return false;
    }
    if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(owner))
        mp_send_character_walk(owner, 0);
    target->ai.attack_point_users[_unk7c] = owner->handle;
    return true;
}

// 0x4bd2a0  Confidence: unsure (the retry loop and the slot-mid switch; the order of the tests matches).
// Out of reach: a target with attack points is approached by point; otherwise the nearest free attacker
// slot of the target (5 directions at the summed radii + attack range - 0.2 around its centre, or around
// its predicted position when it moves and is 5 or more away). When the chosen slot is two steps from the
// one the target would hand out, a waypoint at the slot between is put in front of the path if both legs
// are straight walks. Slots that are not navigable are dropped and the choice repeats. No slot: a path to
// the (nearest navigable) centre, slot -1 claimed for 1 s.
bool ai_state::approach_attack_slot(Character *target)
{
    vector3 dest;
    vector3 dir;
    vector3 mid;
    vector3 slot_pos[5];
    int slot_ids[5];
    vector2 a;
    vector2 b;
    vector2 c;
    float dist;
    float lead;
    float reach;
    float best;
    float d;
    int dest_layer;
    int n;
    int i;
    int best_index;
    int slot;
    int given;
    int diff;
    int mid_slot;
    int ms;
    bool found;
    bool direct;
    Character *mount;
    ai_path *node;

    dist = owner->distance_between_edges(target, true, _unk7c);
    if (within_attack_radius(dist))
        return false;
    dest.x = target->pos.x + target->centre_offset.x;
    dest.y = target->centre_offset.y + target->pos.y;
    dest.z = target->centre_offset.z + target->pos.z;
    dest_layer = target->layer;
    if (target->forward_speed > 0.0f) {
        if (owner->forward_speed < 0.1f)
            lead = 100.0f;
        else
            lead = dist / owner->forward_speed;
    } else {
        lead = 0.0f;
    }
    if (!(dist < 5.0f) && lead > 0.0f) {
        target->ai.predict_owner_position(&dest, 1.0f);
        dest_layer = pf_trace_edge_crossings(&target->pos, target->layer, dest);
    }
    reach = owner->size_radius() + target->size_radius() + attack_range - 0.2f;
    if (character_target_point_count(target) > 0) {
        if (_unk7c != -1) {
            target->ai.attack_point_users[_unk7c] = -1;
            _unk7c = -1;
        }
        return approach_target_point(target);
    }

    n = 0;
    best_index = 0;
    best = 0.0f;
    for (i = 0; i < 5; i++) {
        if (obj_from_handle(target->ai.attackers[i].handle) == NULL
            || target->ai.attackers[i].handle == owner->handle) {
            target->ai.attack_slot_direction(&dir, i);
            slot_ids[n] = i;
            slot_pos[n].x = dir.x * reach + dest.x;
            slot_pos[n].y = dir.y * reach + dest.y;
            slot_pos[n].z = dir.z * reach + dest.z;
            n++;
        }
    }
    for (;;) {
        found = false;
        for (i = 0; i < n; i++) {
            if (slot_ids[i] == -1)
                continue;
            d = distance(&owner->pos, &slot_pos[i]);
            if (!found || d < best) {
                found = true;
                best = d;
                best_index = i;
                dest = slot_pos[i];
            }
        }
        if (!found)
            break;
        direct = false;
        mid_slot = -1;
        given = target->ai.pick_attack_slot(owner);
        slot = slot_ids[best_index];
        if (slot != given) {
            diff = given - slot;
            if (diff <= -3)
                diff += 5;
            else if (diff >= 3)
                diff -= 5;
            if (abs(diff) == 2) {
                switch (slot + given) {
                case 2:
                case 4:
                case 6:
                    mid_slot = (slot + given) / 2;
                    break;
                case 3:
                    mid_slot = 4;
                    break;
                case 5:
                    mid_slot = 0;
                    break;
                }
                target->ai.attack_slot_direction(&dir, mid_slot);
                mid.x = dir.x * reach + (target->pos.x + target->centre_offset.x);
                mid.y = dir.y * reach + (target->centre_offset.y + target->pos.y);
                mid.z = dir.z * reach + (target->centre_offset.z + target->pos.z);
                if (pf_point_is_navigable(&mid, owner->info->size, dest_layer) == true) {
                    a.from_xz(&owner->pos);
                    b.from_xz(&mid);
                    c.from_xz(&dest);
                    if (pf_straight_walk_clear(&a, owner->layer, &b, dest_layer, 3) == true
                        && pf_straight_walk_clear(&b, dest_layer, &c, dest_layer, 3) == true)
                        direct = true;
                }
            }
        }
        if (pf_point_is_navigable(&dest, owner->info->size, dest_layer) != true) {
            slot_ids[best_index] = -1;
            continue;
        }
        if (path_find_new(&owner->pos, owner->layer, &dest, dest_layer, owner->info->size, 0.0f)) {
            if (direct && owner->get_mount() == NULL) {
                node = ai_path_node_create_b(mid, dest_layer, path);
                if (node) {
                    node->dir.x = node->pos.x - owner->pos.x;
                    node->dir.y = node->pos.y - owner->pos.y;
                    node->dir.z = node->pos.z - owner->pos.z;
                    node->dir.y = 0.0f;
                    node->dir.normalize_safe();
                    node->flags |= 2;
                    path->dir.x = path->pos.x - node->pos.x;
                    path->dir.y = path->pos.y - node->pos.y;
                    path->dir.z = path->pos.z - node->pos.z;
                    path->dir.y = 0.0f;
                    path->dir.normalize_safe();
                    path = node;
                }
            }
            mount = owner->get_mount();
            if (mount)
                mount->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
            if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(owner))
                mp_send_character_walk(owner, 0);
        }
        if (found) {
            ms = (int)((best * 0.3f < 2.5f ? best * 0.3f : 2.5f) * 1000.0f);
            target->ai.claim_attack_slot(target, owner, ms, slot_ids[best_index]);
            return true;
        }
    }
    if (!pf_point_is_navigable(&dest, owner->info->size, dest_layer))
        pf_find_nearest_navigable_point(&dest, owner->info->size, dest_layer);
    if (path_find_new(&owner->pos, owner->layer, &dest, dest_layer, owner->info->size, reach)) {
        mount = owner->get_mount();
        if (mount)
            mount->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
        if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(owner))
            mp_send_character_walk(owner, 0);
    }
    target->ai.claim_attack_slot(target, owner, 1000, -1);
    return true;
}

// 0x4bd9c0  Confidence: exact. Remembers where the target is, re-checks in 0.5 s; an attack slot approach
// (AIS_GET_CLOSE) unless a spell is chosen or the caller wants the offset approach (AIS_GET_FAR).
bool ai_state::combat_start_approach(Character *target, bool random_offset)
{
    bool r;

    approach_pos = target->pos;
    approach_layer = target->layer;
    timer30.SetTimeout(500);
    if (mode_target == -1 && !random_offset) {
        r = approach_attack_slot(target);
        if (r == true)
            set_mode(AI_STATE_COMBAT, AIS_GET_CLOSE);
        return r;
    }
    r = approach_random_offset(target);
    if (r == true)
        set_mode(AI_STATE_COMBAT, AIS_GET_FAR);
    return r;
}

// 0x4bda50  Confidence: likely (the reuse of the argument slot for the "use reach" byte is the compiler's).
// The combat decision: no target -> idle; a remote multiplayer character does nothing (the return value is
// what AL holds: false); a monster beyond its fov range drops the attack goal; in reach -> the cast goal,
// the choose-spell sub-mode (combat styles 4 / 5), ranged or melee; else approach.
bool ai_state::combat_approach(Character *target)
{
    float dist;
    float range;
    float min_range;
    float max_range;
    bool ranged;
    bool ranged_approach;
    bool crowded;
    bool use_reach;
    int style;

    if (!target) {
        go_idle();
        return true;
    }
    crowded = false;
    use_reach = true;
    style = get_combat_style();
    if ((game_flags & GF_MULTIPLAYER) && !character_is_locally_controlled(owner))
        return false;
    dist = owner->distance_between_edges(target, true, _unk7c);
    if (owner->type != CHAR_TYPE_PARTY && dist > owner->info->fov_range) {
        goal_remove(goal_find(5, AI_GOAL_ATTACK, 0, 0, target->handle, NULL, 0, AI_NO_SPELL));
        go_idle();
        return true;
    }
    ranged = owner->get_attack_range(&range);
    ranged_approach = ranged;
    if (owner->type != CHAR_TYPE_BOSS) {
        if (ranged == true)
            attack_range = range;
        else
            attack_range = owner->info->attack_radius;
    } else if (owner->info->species == SPECIES_TIGER_RIDER) {
        use_reach = false;
        if (ranged == true)
            attack_range = range;
        else
            attack_range = owner->info->attack_radius;
    } else {
        attack_range = owner->info->attack_radius;
        boss_get_move_range(&max_range, &min_range);
    }
    if (dist > attack_range && character_target_point_count(target) > 0 && _unk7c == -1 && !ranged)
        return combat_start_approach(target, false);
    if (use_reach && ranged == true && owner->info->attack_radius + 0.2f > dist) {
        if (target->ai.count_engaged_attackers(true) > 1) {
            crowded = ranged;
        } else {
            ranged = false;
            ranged_approach = false;
            attack_range = owner->info->attack_radius;
        }
    }
    if (within_attack_radius(dist) && !crowded) {
        if (mode_target != -1)
            return goal_do_cast_at_target(mode_target, this->target) == 0;
        if (!(flags & 0x80) && (style == 4 || style == 5))
            return set_mode(AI_STATE_COMBAT, AIS_CHOOSE_SPELL) != 0;
        if (ranged)
            return set_mode(AI_STATE_COMBAT, AIS_RANGED_COMBAT) != 0;
        return set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT) != 0;
    }
    return combat_start_approach(target, ranged_approach);
}

// 0x4bdce0  Confidence: likely (the flag arithmetic). Sub-mode AIS_TRANSITION: switches between the normal
// and the ready stance. Same anims: just flips the flag. Otherwise, standing still: the stance move
// (move category 10: 0x23 / 0x24 to / from ready, 0x25 / 0x26 the hurt ones) or, without one, a 0.5 s
// cosine blend of stance_blend; the flag flips half way; the weapon prop is hidden / shown at trigger 10.
// True when done.
bool ai_state::combat_toggle_ready_stance()
{
    vector3 v;
    move_info *m;
    float t;
    float s;
    int move;
    bool ready;
    bool flip = false;

    if (owner->anim_stand == owner->anim_ready) {
        owner->flags ^= CF_COMBAT_READY;
        return true;
    }
    ai_move_flags |= 4;
    v = owner->instance->sum_root_velocity();
    if (v.len() > 0.1f)
        return false;
    if (!disable_timer.valid()) {
        flags &= ~0x10000000;
        if (owner->flags & CF_COMBAT_READY)
            flags &= ~0x100;
        else
            flags |= 0x100;
        move = 0;
        if (owner->is_badly_hurt()) {
            move = (owner->flags & CF_COMBAT_READY) ? 0x26 : 0x25;
            if (!owner->find_move(10, move))
                move = 0;
        }
        if (!move)
            move = (owner->flags & CF_COMBAT_READY) ? 0x24 : 0x23;
        disable_timer.SetTimeout(0);
        m = owner->find_move(10, move);
        emerge_move = -1;
        if (m) {
            emerge_move = owner->play_move((int)m, 1.0f, true, true, false);
            if (emerge_move == -1)
                return false;
            if (owner->flags & CF_COMBAT_READY)
                owner->stance_blend = 0.0f;
            else
                owner->stance_blend = 1.0f;
        }
        flags = (flags & ~0x80000002) | 2;
    }
    if (emerge_move >= 0) {
        if (owner->move_finished(emerge_move)) {
            if (flags & 0x100) {
                owner->flags |= CF_COMBAT_READY;
                if (owner->prop_tag != 0)
                    owner->prop_tag = 0;
            } else {
                owner->flags &= ~CF_COMBAT_READY;
                if (owner->prop_tag != owner->default_prop_tag)
                    owner->prop_tag = owner->default_prop_tag;
            }
            flags &= ~0x102;
            return true;
        }
        if (owner->move_trigger_fired(10, 1)) {
            switch (owner->get_current_move()->id) {
            case 0x23:
            case 0x25:
                owner->prop_tag = 0;
                break;
            case 0x24:
            case 0x26:
                owner->prop_tag = owner->default_prop_tag;
                break;
            }
        }
        if (owner->move_progress(emerge_move) >= 0.5f && !(flags & 0x80000000)) {
            flags |= 0x80000000;
            owner->flags ^= CF_COMBAT_READY;
            if (owner->flags & CF_COMBAT_READY)
                owner->stance_blend = 0.0f;
            else
                owner->stance_blend = 1.0f;
            return false;
        }
        return false;
    }
    t = disable_timer.GetElapsedTime() * 0.002f;
    if (t >= 0.5f && !(flags & 0x80000000)) {
        flags |= 0x80000000;
        owner->flags ^= CF_COMBAT_READY;
    }
    if (t >= 1.0f) {
        flags &= ~2;
        return true;
    }
    ready = (owner->flags & CF_COMBAT_READY) ? true : flip;
    if (flags & 0x80000000) {
        flip = !ready;
        ready = flip;
    }
    s = ((float)cos(t * 3.1415927f) + 1.0f) * 0.5f;
    if (ready)
        owner->stance_blend = 1.0f - s;
    else
        owner->stance_blend = s;
    return false;
}

// 0x4be0a0  Confidence: likely. Luminar's grab approach: 1 in range (stops the owner), else a path to where
// the target will be in a second (re-planned every 0.5 s, the range as the search's cost bias), -1 without a
// path or when the path ends and the owner stands still.
int ai_state::chase_target_until_in_range(Character *target, float range)
{
    vector3 pred;
    int layer;

    if (!target)
        return -1;
    if (owner->distance_between_edges(target, false, -1) <= range) {
        owner->set_6ac(0.0f);
        return 1;
    }
    if (path == NULL || timer30.elapsed()) {
        approach_pos = target->pos;
        approach_layer = target->layer;
        target->ai.predict_owner_position(&pred, 1.0f);
        layer = pf_trace_edge_crossings(&target->pos, target->layer, pred);
        if (!pf_point_is_navigable(&pred, owner->info->size, layer)) {
            pred = target->pos;
            layer = target->layer;
        }
        path_find_new(&owner->pos, owner->layer, &pred, layer, owner->info->size, range);
        if (path == NULL)
            return -1;
        if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(owner))
            mp_send_character_walk(owner, 0);
        target->ai.claim_attack_slot(target, owner, 1000, -1);
        timer30.SetTimeout(500);
    }
    ai_follow_target = target;
    if (!follow_path(0) && owner->forward_speed < 0.01f)
        return -1;
    return 0;
}

// 0x4be280  Confidence: likely. Sub-modes AIS_GET_CLOSE / AIS_GET_FAR: without a path (the mount's when
// mounted) decide again; in reach (between half and the whole attack range when ranged, boss ranges
// applied, less than 3 apart in height) switch to the attack sub-mode; else follow the path, re-deciding
// every 0.5 s unless an approach is pending.
void ai_state::combat_close_in(Character *target)
{
    Character *mount;
    float dist;
    float min_range;
    float max_range;
    float dy;
    bool ranged;

    if (!target) {
        go_idle();
        return;
    }
    mount = owner->get_mount();
    if (mount) {
        if (mount->ai.path == NULL) {
            combat_approach(target);
            return;
        }
    } else if (path == NULL) {
        combat_approach(target);
        return;
    }
    ranged = owner->get_attack_range(&attack_range);
    dist = owner->distance_between_edges(target, true, _unk7c);
    if (ranged == true)
        min_range = attack_range * 0.5f;
    else
        min_range = 0.0f;
    max_range = attack_range;
    dy = (float)fabs(owner->pos.y - target->pos.y);
    boss_get_move_range(&min_range, &max_range);
    if (dist <= 0.01f)
        dist = 0.01f;
    if (dist <= max_range && !(dist < min_range) && dy < 3.0f) {
        if (!ranged) {
            set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT);
            return;
        }
        if (!(flags & 0x80) && (combat_style == 4 || combat_style == 5)) {
            set_mode(AI_STATE_COMBAT, AIS_CHOOSE_SPELL);
            return;
        }
        set_mode(AI_STATE_COMBAT, AIS_RANGED_COMBAT);
        return;
    }
    ai_follow_target = target;
    if (!follow_path(0)) {
        if (owner->is_rider()) {
            mount = owner->get_mount();
            if (mount)
                mount->ai.go_mount_idle();
        }
        combat_approach(target);
        return;
    }
    if (timer30.elapsed()) {
        if (!(flags & 0x10000000))
            combat_approach(target);
        timer30.SetTimeout(500);
    }
}

// 0x4be470  Confidence: likely (the centre goes through a temporary). False while still turning towards the
// target's centre (the mount turns for a rider; Titus keeps his facing while titus_turn_lock). With attack
// points (flag 0x20) a locally controlled owner frees its points and makes its attackers pick new ones.
bool ai_state::combat_face_target(Character *target)
{
    vector3 centre;
    Character *mount;
    Character *c;
    int points;
    int i;
    bool r;

    flags &= ~0x10000000;
    centre.x = target->centre_offset.x + target->pos.x;
    centre.y = target->centre_offset.y + target->pos.y;
    centre.z = target->centre_offset.z + target->pos.z;
    if (owner->get_current_move() == NULL || owner->type == CHAR_TYPE_BOSS) {
        points = flags & 0x20;
        if (!(owner->info->species == SPECIES_TITUS && titus_turn_lock == true)) {
            mount = owner->get_mount();
            if (mount)
                r = mount->ai.turn_towards(&centre, true);
            else
                r = turn_towards(&centre, true);
            if (r == true)
                return false;
        }
        if (character_is_locally_controlled(owner) && points && character_target_point_count(owner)) {
            for (i = 0; i < owner->info->num_target_points; i++)
                attack_point_users[i] = -1;
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (c->ai.target == owner->handle) {
                    c->ai._unk7c = -1;
                    if (!c->ai.is_disabled_any() && (owner->flags & 0x400))
                        c->ai.combat_approach(owner);
                }
            }
        }
    }
    return true;
}

// 0x4be5f0  Confidence: likely. Sub-mode AIS_CLOSE_COMBAT. A target with attack points counts as touching
// (bosses only within 5 by the edges); a ranged attacker crowded or out of its own reach re-decides; a
// moving target counts nearer (x 4/9). Facing (or busy), one melee update with the boss move id, both
// sides take each other's attack slot and the victim's guards are alerted.
void ai_state::combat_melee_attack(Character *target)
{
    float dist;
    float range;
    float min_range;
    float max_range;
    int move;

    if (!(target->ai.flags & 0x20)) {
        dist = owner->distance_between_edges(target, true, _unk7c);
    } else {
        dist = 0.0f;
        if (target->type == CHAR_TYPE_BOSS) {
            dist = owner->distance_between_edges(target, true, -1);
            if (dist < 5.0f)
                dist = 0.0f;
        }
    }
    if (owner->get_attack_range(&range) == true) {
        if (target->ai.count_engaged_attackers(true) > 1 || dist > owner->info->attack_radius) {
            if (combat_approach(target) == true)
                return;
        }
    }
    boss_get_move_range(&max_range, &min_range);
    if (target->is_moving())
        dist *= 0.44444445f;
    if (!within_attack_radius(dist)) {
        if (combat_approach(target) == true)
            return;
    }
    if (combat_face_target(target) || owner->combat.is_busy()) {
        move = boss_get_attack_move_id();
        ai_move_flags |= 4;
        owner->combat.update(move);
        claim_attack_slot(owner, target, 0, -1);
        target->ai.claim_attack_slot(target, owner, 0, -1);
        target->ai.alert_guards_of_target(owner);
    }
}

// 0x4be760  Confidence: likely. Sub-mode AIS_RANGED_COMBAT: without a ranged weapon re-decide; out of reach
// or nearer than half the range (the Tiger Rider: 2) re-decide; facing (or busy) one combat update.
void ai_state::combat_ranged_attack(Character *target)
{
    float range;
    float min_range;
    float max_range;
    float dist;
    float too_near;

    if (!owner->get_attack_range(&range)) {
        combat_approach(target);
        return;
    }
    boss_get_move_range(&max_range, &min_range);
    dist = owner->distance_between_edges(target, true, _unk7c);
    too_near = attack_range * 0.5f;
    if (owner->info->species == SPECIES_TIGER_RIDER)
        too_near = 2.0f;
    if (!within_attack_radius(dist) || dist < too_near) {
        if (combat_approach(target) == true)
            return;
    }
    if (combat_face_target(target) || owner->combat.is_busy())
        owner->combat.update(boss_get_attack_move_id());
}

// 0x4be840  Confidence: exact. Sub-mode AIS_CHARGE (ai_charge_target): run the path; at its end strike.
void ai_state::combat_follow_and_strike(Character *target)
{
    if (path) {
        ai_follow_target = target;
        if (follow_path(0))
            return;
    }
    if (combat_face_target(target) || owner->combat.is_busy())
        owner->combat.update(0);
}

// 0x4be8a0  Confidence: likely. Sub-mode AIS_BACKSTAB: more than 2 away -> idle; facing, one update (-1);
// when the record goes idle, an ordinary attack follows and a target not yet in combat attacks back.
void ai_state::combat_close_strike(Character *target)
{
    if (owner->distance_between_edges(target, true, -1) > 2.0f) {
        go_idle();
        return;
    }
    if (combat_face_target(target) && owner->combat.update(-1) == true) {
        go_idle();
        add_attack(target, 1000);
        if (!target->ai.is_mode(AI_STATE_COMBAT, -1)) {
            target->ai.add_attack(owner, 1000);
            target->ai.process_goals();
        }
    }
}

// 0x4be940  Confidence: likely (cdecl). Statuses counted as curable: blind, silence, curse, sleep,
// paralyze, poison, cripple, slow.
float ai_score_support_targets(boss_target *list, int count, bool heal, bool cure)
{
    float total = 0.0f;
    float score;
    float frac;
    Character *c;
    int i;

    for (i = 0; i < count; i++) {
        c = list[i].character;
        score = 0.0f;
        list[i].flags = 0;
        frac = c->hp / (float)c->get_max_stat(0);
        if (heal && frac < 0.5f) {
            list[i].flags |= AIT_HURT;
            score = 50.0f - frac * 100.0f;
        }
        if (cure && (c->status_active(STATUS_BLIND) || c->status_active(STATUS_SILENCE)
                     || c->status_active(STATUS_CURSE) || c->status_active(STATUS_SLEEP)
                     || c->status_active(STATUS_PARALYZE) || c->status_active(STATUS_POISON)
                     || c->status_active(STATUS_CRIPPLE) || c->status_active(STATUS_SLOW))) {
            score += 10.0f;
            list[i].flags |= AIT_CURABLE;
        }
        list[i].score = score;
        total += score;
    }
    return total;
}

// 0x4bea80  Confidence: likely. Vitalize (monsters only after its global timer) when more than two are hurt;
// otherwise a record is picked by comparing one random number with each normalised score (not with the
// running sum; NOTES.md, game bugs): heal for a hurt record, cure for a curable one.
int ai_state::choose_support_spell(boss_target *list, int count, boss_target **out)
{
    bool heal;
    bool cure;
    bool vitalize;
    float total;
    float r;
    int hurt;
    int i;

    heal = owner->can_cast_spell(SPELL_HEAL);
    cure = owner->can_cast_spell(SPELL_CURE);
    if (owner->type == 5 && !spell_global_timers[SPELL_VITALIZE].elapsed())
        vitalize = false;
    else
        vitalize = owner->can_cast_spell(SPELL_VITALIZE);
    total = ai_score_support_targets(list, count, heal, cure);
    *out = NULL;
    if (!(total > 0.0f))
        return -1;
    hurt = 0;
    for (i = 0; i < count; i++) {
        if (list[i].flags & AIT_HURT)
            hurt++;
        list[i].score /= total;
    }
    if (vitalize && hurt > 2)
        return SPELL_VITALIZE;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r <= list[i].score)
            break;
    }
    if (i == count || i == -1)
        return -1;
    *out = &list[i];
    if (list[i].flags & AIT_HURT)
        return SPELL_HEAL;
    if (list[i].flags & AIT_CURABLE)
        return SPELL_CURE;
    return -1;
}

// 0x4beba0  Confidence: likely (the order of the spell tests). Weight per spell: the sleeps 0 on a sleeping
// or engaged target, 15 when its target is on the caster's team, else 10; ice coffin / freeze 0 on a target
// already in one / paralysed; a party member's fireball only while every ally fighting the target resists
// fire (5 x resistance each, 0 as soon as one does not); x4 for the spell in _unk88. Score: (value - 1) x
// intelligence + 1 (party: intelligence 1), times the weight, minus 5 x cost / AP. Non-negative scores are
// appended with the running total.
void ai_state::rate_spell_targets(int spell, boss_target *t, ai_spell_choice *out, int *count, float *total)
{
    float weight = 0.0f;
    float value;
    float intelligence;
    float score;
    float r;
    Character *c;
    Character *other;
    int i;

    if (!owner->can_cast_spell(spell))
        return;
    if (spell == SPELL_ICY_SLEEP || spell == SPELL_BLUE_IMP_SLEEP) {
        if (t->flags & AIT_ASLEEP) {
            weight = 0.0f;
        } else if (t->character->ai.count_engaged_attackers(false) > 0) {
            weight = 0.0f;
        } else {
            other = living_from_handle(t->character->ai.target);
            if (other && other->get_team() == owner->get_team())
                weight = 15.0f;
            else
                weight = 10.0f;
        }
    } else if (spell == SPELL_ICE_COFFIN) {
        if (t->flags & AIT_ICE_COFFIN)
            weight = 0.0f;
        else
            weight = 10.0f;
    } else if (spell == SPELL_FREEZE) {
        if (t->flags & AIT_PARALYZED)
            weight = 0.0f;
        else
            weight = 10.0f;
    } else if (spell == SPELL_FIREBALL && owner->type == CHAR_TYPE_PARTY
               && t->character->ai.count_engaged_attackers(true) > 0) {
        if (group) {
            weight = 0.0f;
            for (i = 0; i < group->count; i++) {
                c = living_from_handle(group->members[i]);
                if (c && c->ai.is_attacking() && c->ai.target == t->character->handle) {
                    r = c->get_resistance(0);
                    if (!(r > 0.0f)) {
                        weight = 0.0f;
                        break;
                    }
                    weight += r * 5.0f;
                }
            }
        }
    } else {
        weight = 10.0f;
    }
    if (spell == _unk88)
        weight *= 4.0f;
    value = spell_target_value(t->character, spell);
    intelligence = owner->info->intelligence;
    if (owner->type == CHAR_TYPE_PARTY)
        intelligence = 1.0f;
    score = 0.0f;
    if (value > 0.1f)
        score = (value - 1.0f) * intelligence + 1.0f;
    score = score * weight - spells[spell].cost / (float)owner->ap * 5.0f;
    if (score < 0.0f)
        return;
    out[*count].spell = spell;
    *total += score;
    out[*count].score = *total;
    (*count)++;
}

// 0x4bee40  Confidence: likely. Not for party members; 30 %: candidates not yet blind (0x10) / silenced
// (0x20) weigh 10 each, the running sum is multiplied by 1.5 at cur_character (NOTES.md), then a cumulative
// pick: silence for cur_character or a caster (styles 4 / 5), blind for the others.
int ai_state::consider_blind_or_silence(boss_target *list, int count, boss_target **out)
{
    bool blind;
    bool silence;
    float total;
    float r;
    int style;
    int i;

    if (owner->type == CHAR_TYPE_PARTY)
        return -1;
    if (frand() < 0.7f)
        return -1;
    if (owner->type == 5 && !spell_global_timers[SPELL_BLIND].elapsed())
        blind = false;
    else
        blind = owner->can_cast_spell(SPELL_BLIND);
    if (owner->type == 5 && !spell_global_timers[SPELL_SILENCE].elapsed())
        silence = false;
    else
        silence = owner->can_cast_spell(SPELL_SILENCE);
    if (!blind && !silence)
        return -1;
    total = 0.0f;
    for (i = 0; i < count; i++) {
        list[i].score = 0.0f;
        if (blind && !(list[i].flags & AIT_BLIND))
            list[i].flags |= AIT_BLIND_OK;
        if (silence && !(list[i].flags & AIT_SILENCED))
            list[i].flags |= AIT_SILENCE_OK;
        if (list[i].flags & (AIT_BLIND_OK | AIT_SILENCE_OK)) {
            total += 10.0f;
            if (list[i].character == cur_character)
                total *= 1.5f;
            list[i].score += total;
        }
    }
    for (i = 0; i < count; i++)
        list[i].score /= total;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r < list[i].score) {
            *out = &list[i];
            style = list[i].character->ai.get_combat_style();
            if ((list[i].flags & AIT_SILENCE_OK)
                && (list[i].character == cur_character || style == 4 || style == 5))
                return SPELL_SILENCE;
            if ((list[i].flags & AIT_BLIND_OK) && style != 4 && style != 5)
                return SPELL_BLIND;
        }
    }
    return -1;
}

// 0x4befe0  Confidence: likely. Meteor storm / inferno: at least two candidates nearer than the spell's
// distance, and a random roll scaled down by the global recovery still running at least 0.9. The distances
// are the records' +0x04, which ai_combat_cast_frame does not fill for this list (NOTES.md).
int ai_state::consider_area_spell(int spell, boss_target *list, int count)
{
    float r;
    int num_near;
    int i;

    if (owner->type == CHAR_TYPE_PARTY)
        return -1;
    if (!owner->can_cast_spell(spell))
        return -1;
    if (count < 2)
        return -1;
    num_near = 0;
    for (i = 0; i < count; i++) {
        if (list[i].dist < spells[spell].distance)
            num_near++;
    }
    if (num_near < 2)
        return -1;
    r = frand();
    if (!spell_global_timers[spell].elapsed())
        r = (1.0f - (float)spell_global_timers[spell].GetRemainingTime()
                    / (float)(spells[spell].ai_recovery_time * 1000)) * r;
    if (r < 0.9f)
        return -1;
    return spell;
}

// 0x4bf0c0  Confidence: likely. Power leech, weighted by the candidates' AP (cumulative).
int ai_state::consider_power_leech(boss_target *list, int count, boss_target *cur)
{
    float r;
    float total;
    int i;

    if (owner->type == CHAR_TYPE_PARTY)
        return -1;
    if (!owner->can_cast_spell(SPELL_POWER_LEECH))
        return -1;
    r = frand();
    if (!spell_global_timers[SPELL_POWER_LEECH].elapsed())
        r = (1.0f - (float)spell_global_timers[SPELL_POWER_LEECH].GetRemainingTime()
                    / (float)(spells[SPELL_POWER_LEECH].ai_recovery_time * 1000)) * r;
    if (r < 0.9f)
        return -1;
    total = 0.0f;
    for (i = 0; i < count; i++) {
        total += list[i].character->ap;
        list[i].score = total;
    }
    for (i = 0; i < count; i++)
        list[i].score /= total;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r < list[i].score)
            return SPELL_POWER_LEECH;
    }
    return -1;
}

// 0x4bf1d0  Confidence: likely. Death, weighted by attack rating (x1.25 for combat style 3, x1.5 for 5).
int ai_state::consider_death(boss_target *list, int count, boss_target *cur)
{
    float r;
    float total;
    float rating;
    int i;

    if (owner->type == CHAR_TYPE_PARTY)
        return -1;
    if (!owner->can_cast_spell(SPELL_DEATH))
        return -1;
    r = frand();
    if (!spell_global_timers[SPELL_DEATH].elapsed())
        r = (1.0f - (float)spell_global_timers[SPELL_DEATH].GetRemainingTime()
                    / (float)(spells[SPELL_DEATH].ai_recovery_time * 1000)) * r;
    if (r < 0.95f)
        return -1;
    total = 0.0f;
    for (i = 0; i < count; i++) {
        rating = list[i].character->combat.attack_rating();
        switch (list[i].character->ai.get_combat_style()) {
        case 3:
            rating = (float)(rating * 1.25);
            break;
        case 5:
            rating *= 1.5f;
            break;
        }
        total += rating;
        list[i].score = total;
    }
    for (i = 0; i < count; i++)
        list[i].score /= total;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r < list[i].score)
            return SPELL_DEATH;
    }
    return -1;
}

// 0x4bf320  Confidence: likely. The poison elemental's poison, weighted by HP fraction; poisoned records 0.
int ai_state::consider_poison(boss_target *list, int count, boss_target *cur)
{
    float r;
    float total;
    int i;

    if (!owner->can_cast_spell(SPELL_POISON))
        return -1;
    r = frand();
    if (!spell_global_timers[SPELL_POISON].elapsed())
        r = (1.0f - (float)spell_global_timers[SPELL_POISON].GetRemainingTime()
                    / (float)(spells[SPELL_POISON].ai_recovery_time * 1000)) * r;
    if (r < 0.9f)
        return -1;
    total = 0.0f;
    for (i = 0; i < count; i++) {
        if (list[i].flags & AIT_POISONED) {
            list[i].score = 0.0f;
        } else {
            total += list[i].character->hp / (float)list[i].character->get_max_stat(0);
            list[i].score = total;
        }
    }
    for (i = 0; i < count; i++)
        list[i].score /= total;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r < list[i].score)
            return SPELL_POISON;
    }
    return -1;
}

// 0x4bf460  Confidence: likely. The jade golem's beam, weighted by HP fraction above 0.25.
int ai_state::consider_jade_beam(boss_target *list, int count, boss_target *cur)
{
    float r;
    float total;
    float frac;
    int i;

    if (!owner->can_cast_spell(SPELL_JADE_BEAM))
        return -1;
    r = frand();
    if (!spell_global_timers[SPELL_JADE_BEAM].elapsed())
        r = (1.0f - (float)spell_global_timers[SPELL_JADE_BEAM].GetRemainingTime()
                    / (float)(spells[SPELL_JADE_BEAM].ai_recovery_time * 1000)) * r;
    if (r < 0.95f)
        return -1;
    total = 0.0f;
    for (i = 0; i < count; i++) {
        frac = list[i].character->hp / (float)list[i].character->get_max_stat(0);
        if (frac > 0.25f) {
            total += frac;
            list[i].score = total;
        } else {
            list[i].score = 0.0f;
        }
    }
    for (i = 0; i < count; i++)
        list[i].score /= total;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r < list[i].score)
            return SPELL_JADE_BEAM;
    }
    return -1;
}

// 0x4bf5a0  Confidence: likely. Life drain on monsters above level 1 with Character +0x690 below 5,
// weighted 0.5 / (+0x690 + 1); the running total is added to the record's old score instead of stored
// (NOTES.md). Nothing when the total is 0.
int ai_state::consider_life_drain(boss_target *list, int count, boss_target *cur)
{
    float r;
    float total;
    Character *c;
    int i;

    if (!owner->can_cast_spell(SPELL_LIFE_DRAIN))
        return -1;
    r = frand();
    if (!spell_global_timers[SPELL_LIFE_DRAIN].elapsed())
        r = (1.0f - (float)spell_global_timers[SPELL_LIFE_DRAIN].GetRemainingTime()
                    / (float)(spells[SPELL_LIFE_DRAIN].ai_recovery_time * 1000)) * r;
    if (r < 0.95f)
        return -1;
    total = 0.0f;
    for (i = 0; i < count; i++) {
        c = list[i].character;
        if (c->_unk690 < 5 && c->level > 1 && c->type != CHAR_TYPE_PARTY) {
            total += 1.0f / (c->_unk690 + 1.0f) * 0.5f;
            list[i].score += total;
        } else {
            list[i].score = 0.0f;
        }
    }
    if (total == 0.0f)
        return -1;
    for (i = 0; i < count; i++)
        list[i].score /= total;
    r = frand();
    for (i = 0; i < count; i++) {
        if (r < list[i].score)
            return SPELL_LIFE_DRAIN;
    }
    return -1;
}

// 0x4bf700  Confidence: likely. The current target joins the list (at most 10) and becomes *out; the
// statuses become record flags. Two team-0 sides skip the specials and the disabling spells; otherwise
// blind / silence, meteor storm, inferno, power leech, death and the species spells come first. Then a
// weighted pick among icicle, lightning, the sleeps and freeze, ice coffin (monsters, more than one
// candidate), fire arrow and fireball (none of 0x1a / 0x1d / 0x4e in flight).
int ai_state::choose_offensive_spell(boss_target *list, int count, boss_target **out)
{
    ai_spell_choice choices[36];
    Character *t;
    int num_choices;
    float total;
    float r;
    int spell;
    int i;
    bool friendly;

    t = living_from_handle(target);
    for (i = 0; i < count; i++) {
        if (list[i].character == t) {
            *out = &list[i];
            break;
        }
    }
    if (*out == NULL && count < 10) {
        *out = &list[count];
        list[count].character = t;
        count++;
    }
    for (i = 0; i < count; i++) {
        list[i].flags = 0;
        if (list[i].character->has_status(STATUS_SLEEP))
            list[i].flags |= AIT_ASLEEP;
        if (list[i].character->has_status(STATUS_ICE_COFFIN))
            list[i].flags |= AIT_ICE_COFFIN;
        if (list[i].character->has_status(STATUS_PARALYZE))
            list[i].flags |= AIT_PARALYZED;
        if (list[i].character->has_status(STATUS_BLIND))
            list[i].flags |= AIT_BLIND;
        if (list[i].character->has_status(STATUS_SILENCE))
            list[i].flags |= AIT_SILENCED;
        if (list[i].character->has_status(STATUS_POISON))
            list[i].flags |= AIT_POISONED;
    }
    if (!t)
        return -1;
    if (owner->get_team() == 0 && t->get_team() == 0) {
        friendly = true;
    } else {
        friendly = false;
        spell = consider_blind_or_silence(list, count, out);
        if (spell != -1)
            return spell;
        spell = consider_area_spell(SPELL_METEOR_STORM, list, count);
        if (spell != -1)
            return spell;
        spell = consider_area_spell(SPELL_INFERNO, list, count);
        if (spell != -1)
            return spell;
        spell = consider_power_leech(list, count, *out);
        if (spell != -1)
            return spell;
        spell = consider_death(list, count, *out);
        if (spell != -1)
            return spell;
        switch (owner->info->species) {
        case SPECIES_POISON_ELEMENTAL:
            spell = consider_poison(list, count, *out);
            if (spell != -1)
                return spell;
            break;
        case SPECIES_JADE_GOLEM:
            spell = consider_jade_beam(list, count, *out);
            if (spell != -1)
                return spell;
            break;
        case SPECIES_LIFE_DRAINER:
            spell = consider_life_drain(list, count, *out);
            if (spell != -1)
                return spell;
            break;
        }
    }
    num_choices = 0;
    total = 0.0f;
    rate_spell_targets(SPELL_ICICLE, *out, choices, &num_choices, &total);
    rate_spell_targets(SPELL_LIGHTNING, *out, choices, &num_choices, &total);
    if (!friendly) {
        rate_spell_targets(SPELL_ICY_SLEEP, *out, choices, &num_choices, &total);
        rate_spell_targets(SPELL_FREEZE, *out, choices, &num_choices, &total);
        rate_spell_targets(SPELL_BLUE_IMP_SLEEP, *out, choices, &num_choices, &total);
    }
    if (count > 1 && owner->type != CHAR_TYPE_PARTY && !friendly)
        rate_spell_targets(SPELL_ICE_COFFIN, *out, choices, &num_choices, &total);
    rate_spell_targets(SPELL_FIRE_ARROW, *out, choices, &num_choices, &total);
    if (fx_count_spells_1a_1d_4e() < 1)
        rate_spell_targets(SPELL_FIREBALL, *out, choices, &num_choices, &total);
    if (num_choices <= 0)
        return -1;
    for (i = 0; i < num_choices; i++)
        choices[i].score /= total;
    r = frand();
    for (i = 0; i < num_choices; i++) {
        if (r <= choices[i].score)
            break;
    }
    if (i == num_choices || i == -1)
        return -1;
    return choices[i].spell;
}

// 0x4bfaa0  Confidence: likely. Sub-mode AIS_CHOOSE_SPELL. Gives up (flag 0x80, approach) after 7 s, for
// cur_character, without spells (flag 4), silenced, attacked or without a ranged reach. Waits while a cast
// goal of the chosen spell exists, on multiplayer clients (and for party members on the host) and while
// more than two cast effects are tracked. Healers (style 5, not party): heal / cure / vitalize among the
// target's group's second list (when the target is a party member) plus themselves. Then an offensive
// spell among the party (monsters) or the group's second list (party members); out of the spell's
// distance it approaches first.
void ai_state::combat_cast_frame(Character *target)
{
    boss_target list[10];
    boss_target *pick;
    ai_group *g;
    Character *c;
    float range;
    int elapsed;
    int attackers;
    int count;
    int spell;
    int i;
    int self;

    owner->set_6ac(0.0f);
    elapsed = timer.GetElapsedTime();
    if (elapsed < 0)
        return;
    if (elapsed > 7000)
        goto give_up;
    if (owner == cur_character || (flags & 4)) {
        flags |= 0x80;
        combat_approach(target);
        return;
    }
    if (mode_target != -1) {
        if (goal_find(0x10, AI_GOAL_CAST_TARGETED, 0, 0, -1, NULL, 0, mode_target) != -1)
            return;
        if (goal_find(0x10, AI_GOAL_CAST_SPELL, 0, 0, -1, NULL, 0, mode_target) != -1)
            return;
    }
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && !(mp_local_player->flags & 1))
        return;
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && (mp_local_player->flags & 1)
        && owner->type == CHAR_TYPE_PARTY)
        return;
    if (cast_effect_track_count() > 2)
        return;
    flags &= ~0x80;
    attackers = count_attackers();
    if (owner->has_status(STATUS_SILENCE) || attackers > 0)
        goto give_up;
    if (!owner->get_attack_range(&range)) {
        flags |= 0x80;
        combat_approach(target);
        return;
    }
    if (get_combat_style() == 5 && owner->type != CHAR_TYPE_PARTY) {
        count = 0;
        self = 0;
        if (target->type == CHAR_TYPE_PARTY) {
            g = target->ai.group;
            for (i = 0; i < g->num_enemies; i++) {
                c = living_from_handle(g->enemies[i]);
                if (!c)
                    continue;
                if (c == owner)
                    self = 1;
                list[count].character = c;
                list[count].dist = distance(&owner->pos, &c->pos);
                count++;
                if (count == 10)
                    break;
            }
        }
        if (count == 0 || !self) {
            list[count].character = owner;
            list[count].dist = 0.0f;
            count++;
        }
        if (count > 0) {
            pick = NULL;
            spell = choose_support_spell(list, count, &pick);
            if (spell != -1) {
                if (pick == NULL)
                    add_cast_spell(spell, 999999);
                else
                    add_cast_at_target(spell, pick->character, 999999);
                process_goals();
                prev_target = target->handle;
                return;
            }
        }
    }
    pick = NULL;
    count = 0;
    if (owner->type == CHAR_TYPE_PARTY) {
        for (i = 0; i < group->num_enemies; i++) {
            c = living_from_handle(group->enemies[i]);
            if (c)
                list[count++].character = c;
        }
    } else if (cur_character && cur_character->ai.group && cur_character->ai.group->count > 0) {
        for (i = 0; i < cur_character->ai.group->count; i++) {
            c = living_from_handle(cur_character->ai.group->members[i]);
            if (c)
                list[count++].character = c;
        }
    }
    spell = choose_offensive_spell(list, count, &pick);
    if (spell == -1)
        goto give_up;
    mode_target = spell;
    if (owner->distance_between_edges(target, false, -1) > spells[mode_target].distance) {
        combat_approach(target);
        return;
    }
    if (pick)
        add_cast_at_target(mode_target, pick->character, 999999);
    else
        add_cast_spell(mode_target, 999999);
    return;

give_up:
    flags |= 0x80;
    combat_approach(target);
}

// 0x4bfed0  Confidence: likely. On the victim's AI record (ai_combat_melee_attack and the boss melee
// frames): retaliation, then every living entity with a protect goal on the victim attacks the attacker.
// Bosses of species_is_boss_b do not call for help.
void ai_state::alert_guards_of_target(Character *attacker)
{
    Character *c;

    if (owner->type == CHAR_TYPE_BOSS && species_is_boss_b(owner->info->species))
        return;
    retaliate_on_hit(attacker->handle);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->ai.goal_find(5, AI_GOAL_PROTECT, 0, 0, owner->handle, NULL, 0, AI_NO_SPELL) >= 0)
            c->ai.add_attack(attacker, 2000);
    }
}

// 0x4bff60  Confidence: likely (the sub-mode switch: bodies laid out 14, 15, 22/23, 16, 17, 21, 34, 63; the
// byte table 0x4c0214 starts at 14). ai_state mode AI_STATE_COMBAT. During a cutscene only the stance
// change runs. Bosses go to boss_ai_frame. A team-0 owner whose team-0 target is not in an ice coffin
// goes idle (and goes on). Sleeping / ice-coffined targets end the attack unless the target just changed
// (flag 0x10); inactive objects end it too. ai_combat_aim_point gets the target's attack point.
void ai_state::mode_combat_frame()
{
    Character *t;
    vector3 bone_pos;
    matrix bone_orient;
    bool stance_done;
    int g;

    if ((game_flags & GF_CUTSCENE) && submode != AIS_TRANSITION)
        return;
    stance_done = false;
    if (owner->handle == world_mouse_target_handle)
        ai_combat_frames_watched++;
    if (owner->flags & CF_SUMMONLIKE)
        return;
    if (owner->type == CHAR_TYPE_BOSS) {
        boss_ai_frame();
        return;
    }
    t = living_from_handle(target);
    if (submode != AIS_TRANSITION && (!t || (t->flags & CF_HIDDEN))) {
        go_idle();
        return;
    }
    if (owner->get_team() == 0) {
        if (!t)
            goto check_owner;
        if (t->get_team() == 0 && !t->has_status(STATUS_ICE_COFFIN))
            go_idle();
    }
    if (t) {
        if (!(flags & 0x10)) {
            if (t->has_status(STATUS_SLEEP) || t->has_status(STATUS_ICE_COFFIN)) {
                goals_remove_type1();
                go_idle();
                return;
            }
        }
        if (!(t->obj_flags & 1)) {
            go_idle();
            return;
        }
    }
check_owner:
    if (!(owner->obj_flags & 1)) {
        go_idle();
        return;
    }
    if (t) {
        if (_unk7c != -1 && character_target_point_count(t) == 0)
            _unk7c = -1;
        if (_unk7c != -1) {
            t->instance->get_bone_world_transform(t->info->target_points[_unk7c], &t->orient, &t->pos,
                                                  &bone_orient, &bone_pos);
            ai_combat_aim_point.x = bone_pos.x + t->centre_offset.x;
            ai_combat_aim_point.y = bone_pos.y + t->centre_offset.y;
            ai_combat_aim_point.z = bone_pos.z + t->centre_offset.z;
        } else {
            ai_combat_aim_point = t->pos;
        }
    }
    switch (submode) {
    case AIS_TRANSITION:
        stance_done = combat_toggle_ready_stance();
        break;
    case AIS_CLOSE_COMBAT:
        combat_melee_attack(t);
        break;
    case AIS_GET_CLOSE:
    case AIS_GET_FAR:
        combat_close_in(t);
        break;
    case AIS_RANGED_COMBAT:
        combat_ranged_attack(t);
        break;
    case AIS_BACKSTAB:
        combat_close_strike(t);
        break;
    case AIS_CHOOSE_SPELL:
        combat_cast_frame(t);
        break;
    case AIS_22:
        if (!follow_path(0) || timer.GetElapsedTime() > 2500)
            combat_approach(t);
        break;
    case AIS_CHARGE:
        combat_follow_and_strike(t);
        break;
    }
    if (submode == AIS_TRANSITION && stance_done == true) {
        if (owner->type == CHAR_TYPE_PARTY)
            priority = 0;
        if (!(owner->flags & CF_COMBAT_READY)) {
            while ((g = goal_find(1, AI_GOAL_ATTACK, 0, 0, -1, NULL, 0, AI_NO_SPELL)) != -1)
                goal_remove(g);
            go_idle();
            return;
        }
        if (!t) {
            go_idle();
            return;
        }
        combat_approach(t);
    }
}
