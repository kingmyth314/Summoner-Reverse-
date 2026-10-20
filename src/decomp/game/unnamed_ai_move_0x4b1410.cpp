// unnamed_ai_move_0x4b1410.cpp - Sum.exe v1.40, code 0x4b1410-0x4b513f: the AI movement unit (path
// following, wall and character avoidance, turning, the traveling / cast / disabled / hiding / sneaking
// mode handlers, the Turn_anims / turn_scaler commands). The file name is not known (a placeholder, see
// src/decomp/README.md): no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables and .data / .rdata values read
// from Summoner/Sum.exe). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: the initialiser run 0x4b1410-0x4b1529 (table entries 303 / 304, 0x5884c0 / 0x5884c4) after
// the level completion unit (game/unnamed_level_complete_0x4b0960.cpp), to ai_mode_sneaking_frame's RET
// (0x4b5136); the next initialiser run (entry 305, 0x4b5140) opens the AI think unit.
// Data: .data 0x59d6e4-0x59d7c3 (turn_scaler, the turn speed factors, then the literals: the two command
// names and help texts, "Use_turn_anims", the turn_scaler usage and status texts, "Destroying AA after
// emerge anim is over\n"); the bool command's usage / status formats and "TRUE" / "FALSE" are pooled
// (0x588c34-0x588c50). .bss 0x247e840-0x247e8c3.
//
// Profiling: every function with an EH frame (ai_find_nearest_wall_hit, ai_probe_move_for_walls,
// ai_avoid_characters, ai_steer_jitter, ai_path_closest_point, ai_wall_avoid_steer, ai_turn_towards,
// ai_follow_path, ai_travel_steer_update, the traveling and cast-spell frames, ai_cast_spell_update) has
// one unwind action, the empty destructor 0x434650 on a local: a profile_label scope as in ai.cpp and
// pathfinding.cpp (vsdk/os/profile.h); the label texts are not in the binary, the function names are used.
//
// Inline expansion: the big functions call the vector3 constructor (0x43b580), copy constructor
// (0x410cd0) and vector3::dot (0x4b24f0) out of line part of the way through, where the short ones expand
// them; that is VC6's inline budget, not the source (the expressions are written the same way).
// 0x4b24f0 vector3__dot: the COMDAT of the inline vector3::dot (vsdk/math/vector.h, D504) emitted with this
// unit's code. Confidence: exact (no source of its own here).

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/profile.h"
#include "vsdk/os/timer.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/geom/vfx.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/item_paging.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/effects/fx.h"
#include "engine/spells/spells.h"
#include "summoner/summoner.h"
#include "summoner/gametarget.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define AI_WALL_NONE       1.0e9f           // 0x4e6e6b28, ai_wall_hit_dist before a search
#define AI_AVOID_RADIUS    1.25f            // 0x57e994: the radius scale of the collision tests
#define AI_AVOID_SIDE      0.8f             // 0x5819b0
#define AI_TURN_DEG        57.29578f        // 0x580e24
#define AI_TURN_ANIM_TIME  120.0f           // 0x581bac (degrees per turn move at speed 1)
#define AI_TURN_ANIM_SPEED 0.95f            // 0x581980

// Turn moves (move type 9) and the sneak moves (type 10), by variation id.
#define MOVE_TURN_LEFT     0xa5
#define MOVE_TURN_RIGHT    0xa6
#define MOVE_KNOCKED_DOWN  0x8e
#define MOVE_GET_UP        0x90
#define MOVE_STUNNED       0x91
#define MOVE_SNEAK_IN      0x27
#define MOVE_SNEAK_OUT     0x28

// AI sub-modes used here (ai.cpp has the others; names ours).
#define AIS_TRAVEL_PATH        0x00
#define AIS_TRAVEL_STEER       0x01
#define AIS_TRAVEL_TURN        0x02
#define AIS_TRAVEL_FOLLOW      0x03
#define AIS_TRAVEL_FOLLOW_SLOW 0x04
#define AIS_TRAVEL_WAIT        0x05
#define AIS_TRAVEL_GOTO        0x06
#define AIS_TRAVEL_ESCORT      0x07
#define AIS_TRAVEL_WANDER      0x08
#define AIS_ENTER_HIDING       0x09
#define AIS_HIDING             0x0a
#define AIS_LEAVE_HIDING       0x0b
#define AIS_ENTER_SNEAKING     0x0c
#define AIS_LEAVE_SNEAKING     0x0d
#define AIS_COMBAT_BACKSTAB    0x11
#define AIS_CAST_SPELL         0x27
#define AIS_CAST_AT_TARGET     0x28
#define AIS_CAST_AT_POINT      0x29
#define AIS_TRAVEL_ASIDE       0x2b
#define AIS_KNOCKED_DOWN       0x2c
#define AIS_KNOCKED_DOWN_HOLD  0x2d
#define AIS_GET_UP             0x2e
#define AIS_PUSHED             0x2f
#define AIS_STUNNED            0x30
#define AIS_DISABLED_TIMED     0x31

#define SKILL_HIDE_SNEAK       0x16          // the skill whose level times the sneak timeout (Character +0x62a)

// ---- .data -----------------------------------------------------------------------------------------
float turn_scaler = 2.0f;                                     // 0x59d6e4
float ai_turn_speed_factors[11] = {                           // 0x59d6e8
    0.0f, 0.316f, 0.447f, 0.548f, 0.632f, 0.707f, 0.775f, 0.837f, 0.894f, 0.949f, 1.0f,
};

// ---- .bss --------------------------------------------------------------------------------------------
int ai_mode_frame_counts[NUM_AI_STATES];                      // 0x247e840
float ai_path_remaining;                                      // 0x247e874
Character *ai_follow_target;                                  // 0x247e878
float ai_wall_hit_dist;                                       // 0x247e8a0
pf_edge *ai_wall_hit_edge;                                    // 0x247e8a4
int ai_wall_hit_layer;                                        // 0x247e8a8
bool ai_wall_hit_obstacle;                                    // 0x247e8ac
vector2 *ai_wall_hit_a;                                       // 0x247e8b0
vector2 *ai_wall_hit_b;                                       // 0x247e8b4
bool Use_turn_anims;                                          // 0x247e8c0

// ---- console commands ------------------------------------------------------------------------------
// 0x4b1410 static_init_ccmd_Turn_anims (jmp, static initialiser entry 303). Confidence: exact.
// 0x4b1420 init_ccmd_Turn_anims (console_command constructor). Confidence: exact.
console_command ccmd_Turn_anims("Turn_anims", "Toggles Use_turn_anims", cmd_Turn_anims);   // 0x247e890

// 0x4b1440  Confidence: exact (the console's bool command macro).
void cmd_Turn_anims()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Use_turn_anims = true;
        else if (arg_type & ARG_FALSE)
            Use_turn_anims = false;
        else if (arg_type & ARG_NONE)
            Use_turn_anims ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "Turn_anims", "Use_turn_anims");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "Turn_anims", Use_turn_anims ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4b1500 static_init_ccmd_turn_scaler (jmp, static initialiser entry 304). Confidence: exact.
// 0x4b1510 init_ccmd_turn_scaler (console_command constructor). Confidence: exact.
console_command ccmd_turn_scaler("turn_scaler", "Set the turn scaler", cmd_turn_scaler);   // 0x247e880

// 0x4b1530  Confidence: exact.
void cmd_turn_scaler()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        turn_scaler = command_argument_float;
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Turn scaler is %.3f\n", turn_scaler);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: turn_scaler <value>\n", 0);
}

// ---- wall probes -----------------------------------------------------------------------------------
// 0x4b15a0  Confidence: likely (the bounding box as two if / else pairs; the squared distance's operand
// order). Only the walls whose normal faces the move count; the obstacle loop stops after the size
// class's count of prohibit obstacles (avoid_obstacle_counts).
bool ai_find_nearest_wall_hit(vector2 *from, vector2 *to, vector3 *dir, int size, int layer, bool obstacles)
{
    profile_label label("ai_find_nearest_wall_hit");
    bool hit = false;
    float min_x, max_x, min_y, max_y;
    pf_edge move;
    pf_edge edge;
    vector2 pt;
    pf_cell *cell;
    int index;

    ai_wall_hit_edge = NULL;
    ai_wall_hit_dist = AI_WALL_NONE;
    move.a = from;
    move.b = to;
    if (from->x < to->x) {
        min_x = from->x;
        max_x = to->x;
    } else {
        min_x = to->x;
        max_x = from->x;
    }
    if (from->y < to->y) {
        min_y = from->y;
        max_y = to->y;
    } else {
        min_y = to->y;
        max_y = from->y;
    }
    index = size * PF_MAX_LAYERS + layer;
    for (cell = pf_graphs[size][layer].cells; cell; cell = cell->next) {
        pf_edge *e;
        int i;

        if (!(min_x < cell->bounds.max_x) || !(max_x > cell->bounds.min_x)
            || !(min_y < cell->bounds.max_y) || !(max_y > cell->bounds.min_y))
            continue;
        e = cell->edges;
        for (i = 0; i < cell->num_edges; i++, e++) {
            vector2 *a, *b;
            if (e->flags & PF_EDGE_SIDE) {
                a = e->a;
                b = e->b;
            } else {
                a = e->b;
                b = e->a;
            }
            if ((a->y - b->y) * dir->x + (b->x - a->x) * dir->z < 0.0f
                && seg2d_segment_intersect(&move, e, &pt) == 1) {
                float dx = pt.x - from->x;
                float dy = pt.y - from->y;
                float d = dx * dx + dy * dy;
                if (d < ai_wall_hit_dist) {
                    ai_wall_hit_edge = e;
                    ai_wall_hit_dist = d;
                    ai_wall_hit_layer = layer;
                    ai_wall_hit_obstacle = false;
                    hit = true;
                }
            }
        }
    }
    if (obstacles) {
        avoid_obstacle *ob;
        int n = 0;

        for (ob = avoid_obstacles; ob < &avoid_obstacles[AVOID_NUM_OBSTACLES]; ob++) {
            if (n == avoid_obstacle_counts[index])
                break;
            if ((ob->flags & AVOID_OB_USED) && (ob->flags & AVOID_OB_PROHIBIT) && ob->layer == layer
                && ob->size == size) {
                n++;
                if (avoid_segment_hits_prim(ob, from, to, -0.1f)) {
                    int count = ob->num_points;
                    int prev = count - 1;
                    int j;
                    for (j = 0; j < count; j++) {
                        edge.a = &ob->points[prev];
                        edge.b = &ob->points[j];
                        prev = j;
                        if (seg2d_segment_intersect(&move, &edge, &pt) == 1) {
                            float dx = pt.x - from->x;
                            float dy = pt.y - from->y;
                            float d = dx * dx + dy * dy;
                            if (d < ai_wall_hit_dist) {
                                ai_wall_hit_a = edge.a;
                                ai_wall_hit_b = edge.b;
                                ai_wall_hit_dist = d;
                                ai_wall_hit_layer = layer;
                                ai_wall_hit_obstacle = true;
                                hit = true;
                            }
                        }
                    }
                }
            }
        }
    }
    return hit;
}

// 0x4b18e0  Confidence: exact. The crossing layer goes back into the first argument's slot.
bool ai_probe_move_for_walls(vector3 *pos, vector3 *move, int size, int layer, bool obstacles)
{
    profile_label label("ai_probe_move_for_walls");
    vector2 a, b, hit;
    pf_edge seg;
    int next_layer;

    a.from_xz(pos);
    vector3 end(pos->x + move->x, pos->y + move->y, pos->z + move->z);
    b.from_xz(&end);
    seg.a = &a;
    seg.b = &b;
    while (pf_first_edge_crossing(&seg, layer, &hit, &next_layer)) {
        if (ai_find_nearest_wall_hit(&a, &hit, move, size, layer, obstacles))
            return true;
        a = hit;
        layer = next_layer;
    }
    return ai_find_nearest_wall_hit(&a, &b, move, size, layer, obstacles);
}

// 0x4b1a00  Confidence: exact (the test sums the products, the division the named squares: two expressions,
// so VC6 stores the squares and forms the sum twice; D9500).
float closest_approach_time(vector3 *va, vector3 *vb, obj *a, obj *b)
{
    vector3 dv = *vb - *va;
    float xx = dv.x * dv.x;
    float yy = dv.y * dv.y;
    float zz = dv.z * dv.z;
    vector3 dp;

    if (dv.x * dv.x + dv.y * dv.y + dv.z * dv.z < 0.0001f)
        return 0.0f;
    dp = a->pos - b->pos;
    return (dp.z * dv.z + dp.y * dv.y + dp.x * dv.x) / (xx + yy + zz);
}

// ---- steering ----------------------------------------------------------------------------------------
// 0x4b1aa0  Confidence: unsure (the order of the three candidate searches is the binary's; the vector
// temporaries and the dead load of the group's flag byte at 0x4b1b73 are not reproduced). The previous
// avoidance target is kept while it will still collide; else the nearest living entity on a collision
// course within sqrt(50) ahead; else, while following a leader, anyone overlapping now. The owner then
// steers to the side of it that is open (travel_dest one unit sideways) and path_started becomes 2
// (ai_turn_towards uses steer_side).
bool ai_state::avoid_characters()
{
    profile_label label("ai_avoid_characters");
    bool blocked = false;
    Character *avoid = NULL;
    float t_hit;
    float r;
    vector3 vel;
    vector3 other_vel;
    Character *c;

    if (!timer224.elapsed())
        return false;
    if (owner->flags & 0x20)
        return false;
    t_hit = ai_path_remaining - owner->size_radius();
    if (t_hit > 1.0f)
        t_hit = 1.0f;
    owner->get_forward_velocity(&vel);
    if (owner->type == 7 && owner != cur_character) {
        float d2 = distance_squared(&owner->pos, &cur_character->pos);
        r = (cur_character->size_radius() + owner->size_radius()) * AI_AVOID_RADIUS;
        if (d2 < r * r) {
            t_hit = 0.0f;
            avoid = cur_character;
            if (avoid)
                goto steer;
        }
    }
    if (vel.x * vel.x + vel.z * vel.z + vel.y * vel.y < 0.05f)
        return false;

    // the character avoided last time
    c = living_from_handle(avoid_handle);
    if (c && !(c->flags & 0x20)) {
        vector3 d = c->pos - owner->pos;
        if (d.z * owner->orient.fvec.z + d.y * owner->orient.fvec.y + d.x * owner->orient.fvec.x > 0.0f) {
            float t;
            c->get_forward_velocity(&other_vel);
            t = closest_approach_time(&vel, &other_vel, owner, c);
            if (t > 0.0f && t < t_hit) {
                vector3 cp = c->pos + other_vel * t;
                vector3 op = owner->pos + vel * t;
                float d2 = distance_squared(&op, &cp);
                r = (c->size_radius() + owner->size_radius()) * AI_AVOID_RADIUS;
                if (d2 < r * r)
                    blocked = true;
            }
        }
    }

    // the nearest one on a collision course
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        int target_handle, mount_handle;
        Character *t;

        if (c->flags & 0x22)
            continue;
        target_handle = owner->ai.target;
        mount_handle = -1;
        t = living_from_handle(target_handle);
        if (t) {
            Character *m = t->get_mount();
            if (m)
                mount_handle = m->handle;
        }
        if (c == owner || c->handle == target_handle || c->handle == mount_handle)
            continue;
        if (!(distance_squared(&owner->pos, &c->pos) < 50.0f))
            continue;
        {
            vector3 d = c->pos - owner->pos;
            float tc;
            if (!(d.z * owner->orient.fvec.z + d.y * owner->orient.fvec.y + d.x * owner->orient.fvec.x > 0.0f))
                continue;
            c->get_forward_velocity(&other_vel);
            tc = closest_approach_time(&vel, &other_vel, owner, c);
            if (tc > 0.0f && tc < t_hit) {
                vector3 cp = c->pos + other_vel * tc;
                vector3 op = owner->pos + vel * tc;
                float d2 = distance_squared(&op, &cp);
                r = (c->size_radius() + owner->size_radius()) * AI_AVOID_RADIUS;
                if (d2 < r * r) {
                    avoid = c;
                    t_hit = tc;
                }
            }
        }
    }
    if (avoid)
        goto steer;

    // following someone: anyone overlapping now
    if (is_mode(AI_STATE_TRAVELING, AIS_TRAVEL_FOLLOW)) {
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            float d2;
            if (c->flags & 0x20)
                continue;
            if (c == owner)
                continue;
            d2 = distance_squared(&owner->pos, &c->pos);
            r = (c->size_radius() + owner->size_radius()) * AI_AVOID_RADIUS;
            if (d2 < r * r) {
                t_hit = 0.0f;
                avoid = c;
            }
        }
        if (avoid)
            goto steer;
    }
    return blocked;

steer:
    {
        vector3 ap, op, diff, dest, push;
        float side, s, dist, overlap, gap;

        avoid_handle = avoid->handle;
        avoid->get_forward_velocity(&other_vel);
        blocked = false;
        ap = avoid->pos + other_vel * t_hit;
        op = owner->pos + vel * t_hit;
        diff = ap - op;
        side = (owner->size_radius() + avoid->size_radius()) * AI_AVOID_SIDE;
        s = diff.z * owner->orient.rvec.z + diff.y * owner->orient.rvec.y + diff.x * owner->orient.rvec.x;
        if (s > 0.0f)
            dest = ap - owner->orient.rvec * side;
        else
            dest = ap + owner->orient.rvec * side;
        if (!pf_point_is_navigable(&dest, owner->info->size, avoid->layer)) {
            blocked = true;
            if (s > 0.0f)
                dest = ap + owner->orient.rvec * side;
            else
                dest = ap - owner->orient.rvec * side;
            if (!pf_point_is_navigable(&dest, owner->info->size, avoid->layer))
                blocked = false;
        }
        dist = (float)sqrt(diff.z * diff.z + diff.y * diff.y + diff.x * diff.x);
        diff.normalize_safe();
        overlap = (avoid->size_radius() + owner->size_radius()) * 2.0f;
        gap = overlap - dist;
        if (gap < 0.0f)
            gap = 0.0f;
        push = vector3(diff.x * gap, diff.y * gap, diff.z * gap);
        steer_side = -((push.z * owner->orient.rvec.z + push.y * owner->orient.rvec.y + push.x * owner->orient.rvec.x)
                       * owner->info->moving_turn_rate) / overlap;
        if (blocked)
            steer_side *= -1.5f;
        if (diff.z * owner->orient.rvec.z + diff.y * owner->orient.rvec.y + diff.x * owner->orient.rvec.x < 0.0f)
            travel_dest = owner->pos + owner->orient.rvec;
        else
            travel_dest = owner->pos - owner->orient.rvec;
        if (diff.dot(owner->orient.fvec) > 0.5f && t_hit < 0.5f)
            path_flags |= 1;
        path_started = 2;
    }
    return true;
}

// 0x4b2510  Confidence: likely (the clamps reflect the overshoot back into the band).
void ai_state::steer_jitter(float dt)
{
    float limit = dt * 0.05f;
    float decay = 0.025f / dt;
    profile_label label("ai_steer_jitter");

    steer_offset *= 1.0f - decay;
    steer_offset = frand_range(-0.015f, 0.015f) + steer_offset;
    if (steer_offset > limit)
        steer_offset = limit - (steer_offset - limit);
    if (-limit > steer_offset)
        steer_offset = -limit - (limit + steer_offset);
    travel_dest = owner->get_center() + owner->orient.fvec * 2.0f + owner->orient.rvec * steer_offset;
}

// 0x4b2670  Confidence: likely. The look-ahead point is two seconds of the forward speed along the facing;
// the closest point on the first five path segments (from 50 units behind the first node) replaces
// travel_dest's x / z when it is farther than max_dist. q is initialised in its declaration (an assignment
// went through operator= and rounded the other component of dir * 50) and seg.b is set in the loop (D9510).
void ai_state::path_closest_point(float max_dist)
{
    profile_label label("ai_path_closest_point");
    bool found = false;
    int n = 5;
    float best_d = 0.0f;
    vector2 closest(0.0f, 0.0f);
    vector2 best(0.0f, 0.0f);
    vector2 look, prev, cur;
    seg2d seg;
    ai_path *node;

    timer_get(1000000);
    if (!path)
        return;
    {
        float ahead = owner->forward_speed * 2.0f / owner->speed_scale;
        vector3 p = owner->get_center() + owner->orient.fvec * ahead;
        look.from_xz(&p);
        vector3 q = path->pos - path->dir * 50.0f;
        prev.from_xz(&q);
    }
    seg.a = &prev;
    for (node = path; node; node = node->next) {
        float dx, dy, d;
        if (n-- == 0)
            break;
        cur.from_xz(&node->pos);
        seg.b = &cur;
        seg2d_closest_point(&look, &seg, &closest);
        dx = look.x - closest.x;
        dy = look.y - closest.y;
        d = dx * dx + dy * dy;
        if (!found || d < best_d) {
            best_d = d;
            best = closest;
            found = true;
        }
        prev = cur;
    }
    if (found && best_d > max_dist) {
        travel_dest.x = best.x;
        travel_dest.z = best.y;
    }
}

// 0x4b2890  Confidence: likely (m-1020 w11, D12008: the centre through vector3::set, evaluated z, y, x as the
// binary). A wall nearer than the next path node turns the owner away from it:
// steer_side is +-1 / (d + 1)^2 times the moving turn rate, and the timer 0x224 keeps the steering for
// 500 ms (path_flags bit 1).
void ai_state::wall_avoid_steer()
{
    profile_label label("ai_wall_avoid_steer");
    Character *o;
    float look, d2, k;
    vector3 move, center;

    if (owner->forward_speed < 0.01f)
        goto clear;
    o = owner;
    look = o->get_speed() / o->speed_scale * 2.5f + 0.1f;
    look = min(look, 2.0f);
    move = o->orient.fvec * look;
    center.set(o->pos.x + o->centre_offset.x, o->pos.y + o->centre_offset.y, o->pos.z + o->centre_offset.z);
    {
        bool hit = ai_probe_move_for_walls(&center, &move, o->info->size, o->layer, false);
        d2 = ai_wall_hit_dist;
        if (!hit)
            goto clear;
    }
    if (ai_path_remaining * ai_path_remaining <= d2)
        goto clear;
    travel_dest = path->pos;
    if (!(path_flags & 2)) {
        float tx = path->pos.x - owner->pos.x - owner->centre_offset.x;
        float tz = path->pos.z - owner->pos.z - owner->centre_offset.z;
        float side = tx * owner->orient.fvec.z - tz * owner->orient.fvec.x;
        if (side == 0.0f) {
            path_flags &= ~2;
            return;
        }
        steer_side = side < 0.0f ? -1.0f : 1.0f;
    }
    k = 1.0f / ((d2 + 1.0f) * (d2 + 1.0f)) * owner->info->moving_turn_rate;
    if (steer_side < 0.0f)
        k = -k;
    steer_side = k;
    if (d2 < 1.0f)
        path_flags |= 1;
    steer_offset = 0.0f;
    path_flags |= 2;
    timer224.SetTimeout(500);
    return;

clear:
    path_flags &= ~2;
}

// 0x4b2af0  Confidence: likely (the three rotations use one zeroed origin each; `want` is turned by the
// boss rate the other way). Returns true while turning, false when facing the point (the forward is then
// set to it) or when the point is the owner's own position. While standing, Use_turn_anims plays the
// turn-in-place moves for turns wider than 45 degrees and paces the turn by the move.
bool ai_state::turn_towards(vector3 *point, bool boss_rate)
{
    profile_label label("ai_turn_towards");
    float dt = owner->frametime;
    float max_turn, side, cos_a, angle, t;
    vector3 d, want, fwd, right;

    if (owner->is_mount() && owner->frametime == 0.0f) {
        Character *rider = owner->get_rider();
        if (rider)
            dt = rider->frametime;
    }
    flags &= ~0x20;
    if (path_started == 2) {
        max_turn = dt * steer_side;
        goto apply;
    }
    d.x = point->x - owner->pos.x + owner->centre_offset.x;
    d.y = 0.0f;
    d.z = point->z - owner->pos.z + owner->centre_offset.z;
    if (d.normalize_length() == 0.0f)
        return false;
    want = d;
    fwd = owner->orient.fvec;
    right = owner->orient.rvec;
    if (boss_rate) {
        float rate = boss_turn_rate_override();
        if (rate != 0.0f) {
            {
                vector3 origin(0.0f, 0.0f, 0.0f);
                vector3_rotate_about_axis(&fwd, &owner->orient.fvec, rate, &origin, &owner->orient.uvec);
            }
            {
                vector3 origin(0.0f, 0.0f, 0.0f);
                vector3_rotate_about_axis(&right, &owner->orient.rvec, rate, &origin, &owner->orient.uvec);
            }
            {
                vector3 origin(0.0f, 0.0f, 0.0f);
                vector3_rotate_about_axis(&want, &d, -rate, &origin, &owner->orient.uvec);
            }
        }
    }
    cos_a = fwd.z * d.z + fwd.x * d.x + fwd.y * d.y;
    side = want.z * owner->orient.rvec.z + want.y * owner->orient.rvec.y + want.x * owner->orient.rvec.x;
    if (cos_a >= 1.0f)
        cos_a = 1.0f;
    else if (cos_a <= -1.0f)
        cos_a = -1.0f;
    angle = (float)acos(cos_a);
    t = 0.0f;
    if (owner->forward_speed < 0.01f) {
        char_info *info = owner->info;
        max_turn = dt * info->fast_turn_rate;
        if (cos_a > 0.3f)
            max_turn = dt * info->slow_turn_rate;
        if (Use_turn_anims == true && cos_a < 0.707f) {
            if (owner->move_playing(turn_move, NULL)) {
                int id = owner->get_current_move()->id;
                if ((id == MOVE_TURN_LEFT && side >= 0.0f) || (id == MOVE_TURN_RIGHT && side < 0.0f)) {
                    owner->instance->clear_action_anims();
                    turn_move = -1;
                }
            }
            if (!owner->move_playing(turn_move, &t)) {
                move_info *m = owner->find_move(9, side < 0.0f ? MOVE_TURN_LEFT : MOVE_TURN_RIGHT);
                if (!m)
                    goto compare;
                turn_move = owner->play_move((int)m, 1.0f, false, false, false);
                owner->move_playing(turn_move, &t);
                owner->instance->set_anim_speed(m->anim,
                                                AI_TURN_ANIM_SPEED * (AI_TURN_ANIM_TIME / (angle * AI_TURN_DEG)));
            }
            max_turn = angle / t * dt;
        }
    } else {
        max_turn = dt * owner->info->moving_turn_rate;
    }
compare:
    if (max_turn > angle) {
        owner->orient.fvec = want;
        owner->orient.orthogonalize();
        return false;
    }
    if (side < 0.0f)
        max_turn = -max_turn;
apply:
    if (owner->ai.flags & 0x10000000)
        max_turn = turn_scaler * max_turn;
    {
        matrix rot;
        rot.set_from_angles(0.0f, 0.0f, max_turn);
        owner->orient = owner->orient * rot;
    }
    owner->orient.orthogonalize();
    flags |= 0x20;
    return true;
}

// ---- path following ----------------------------------------------------------------------------------
// 0x4b3010  Confidence: unsure (the long shape is the binary's; the vector temporaries and some operand
// orders are guesses). Once a second the first node is dropped when the second one is in straight
// walking reach; every second (party members 200 ms) the move to the first node is probed for walls and
// a corner node is put in front of it (or the path is searched again); then the owner steers and turns
// toward the node, pops the nodes it passed, and sets the speed from the remaining distance, the next
// turn and the stopping distance. Returns false when the path is gone.
bool ai_state::follow_path(float speed)
{
    profile_label label("ai_follow_path");
    Character *mount = owner->get_mount();
    vector3 target;
    vector3 dir;
    float dist, first_dist;
    bool r;

    if (mount)
        return mount->ai.follow_path(speed);
    path_flags &= ~1;
    pf_select_graph(owner->info->size, owner->layer);

    if (path_timer_a.elapsed()) {
        path_timer_a.SetTimeout(1000);
        if (path && path->next && !(path->flags & 3)) {
            vector2 a, b;
            a.from_xz(&owner->pos);
            b.from_xz(&path->next->pos);
            if (pf_straight_walk_clear(&a, owner->layer, &b, path->next->layer, 7)) {
                path_start = path->pos;
                path = ai_path_pop_node(path);
                path->dir = path->pos - owner->pos;
                path->dir.y = 0.0f;
                path->dir.normalize_safe();
            }
        }
    }

    if (path_timer_b.elapsed()) {
        path_timer_b.SetTimeout(owner->type == 7 ? 200 : 1000);
        path_flags &= ~0x10;
        if (path) {
            vector3 move = path->pos - owner->pos;
            vector3 from = owner->pos;
            if (ai_probe_move_for_walls(&from, &move, pf_cur_size, owner->layer, true)) {
                vector2 node, corner;
                ai_path *np;

                node.from_xz(&path->pos);
                corner = ai_wall_hit_obstacle ? *ai_wall_hit_a : *ai_wall_hit_edge->a;
                if (pf_straight_walk_clear(&corner, ai_wall_hit_layer, &node, path->layer, 7))
                    goto insert;
                corner = ai_wall_hit_obstacle ? *ai_wall_hit_b : *ai_wall_hit_edge->b;
                if (pf_straight_walk_clear(&corner, ai_wall_hit_layer, &node, path->layer, 7))
                    goto insert;
                if (ai_wall_hit_obstacle == true) {
                    path_flags |= 0x10;
                } else {
                    ai_path *last = path;
                    while (last->next)
                        last = last->next;
                    np = ai_path_find(&owner->pos, owner->layer, &last->pos, last->layer, owner->info->size, 0.0f);
                    ai_path_free(path);
                    path = np;
                }
                goto probed;
            insert:
                np = ai_path_node_create_b(corner.to_vector3(), ai_wall_hit_layer, path);
                if (np) {
                    np->dir = np->pos - owner->pos;
                    np->dir.y = 0.0f;
                    np->dir.normalize_safe();
                    path->dir = path->pos - np->pos;
                    path->dir.y = 0.0f;
                    path->dir.normalize_safe();
                    path = np;
                }
            }
        }
    }
probed:
    if (path_flags & 0x10) {
        owner->set_6ac(0.0f);
        return true;
    }
    if (!path) {
        flags &= ~0x10000000;
        return false;
    }
    target = path->pos;
    if (path_started == 1 && !(flags & 0x10000000)) {
        travel_dest = target;
        if (!turn_towards(&travel_dest, false))
            path_started = 0;
        ai_move_flags = 6;
        return true;
    }

    dir.x = target.x - owner->pos.x;
    dir.y = 0.0f;
    dir.z = target.z - owner->pos.z;
    dist = (float)sqrt(dir.x * dir.x + dir.z * dir.z);
    if (dist != 0.0f)
        dir.normalize_safe();
    else
        dir = owner->orient.fvec;
    ai_path_remaining = dist + path->cost;
    prev_path_started = path_started;
    path_started = 0;

    if (!(flags & 0x10000000) && !(owner->forward_speed < 0.01f)) {
        if (Ai_path_wandering_enabled && !(path_flags & 8)) {
            if (!ai_path_flag_59af59 || !avoid_characters()) {
                float off;
                travel_dest = target;
                off = (ai_path_remaining - 0.75f) * 0.2f;
                if (off < 0.0f)
                    off = 0.0f;
                else if (off > 1.5f)
                    off = 1.5f;
                if (is_mode(AI_STATE_TRAVELING, AIS_TRAVEL_FOLLOW))
                    off = 0.0f;
                if (perception_timer.elapsed()) {
                    perception_timer.SetTimeout(25);
                    steer_jitter(0.5f);
                    path_closest_point(off);
                }
            }
            wall_avoid_steer();
            if (dir.z * owner->orient.fvec.z + dir.y * owner->orient.fvec.y + dir.x * owner->orient.fvec.x < 0.0f) {
                travel_dest = target;
                path_flags |= 1;
            }
            if (path && !path->next)
                travel_dest = target + path->dir * 0.5f;
            r = turn_towards(&travel_dest, false);
        } else {
            if (!ai_path_flag_59af59 || !avoid_characters())
                travel_dest = target;
            if (path && !path->next)
                travel_dest = target + path->dir * 0.5f;
            wall_avoid_steer();
            r = turn_towards(&travel_dest, false);
        }
    } else {
        travel_dest = target;
        if (!(flags & 0x10000000))
            wall_avoid_steer();
        r = turn_towards(&travel_dest, false);
        if (!r)
            goto facing;
        path_flags |= 1;
    }
    if (r) {
        if (owner->forward_speed < 0.01f) {
            ai_move_flags = 6;
            return true;
        }
        goto moving;
    }
facing:
    flags &= ~0x10000000;
moving:
    if (!is_attacking())
        ai_move_flags |= 2;
    owner->instance->get_root_velocity();

    // pop the nodes behind the owner
    dir.x = path->pos.x - owner->pos.x;
    dir.y = 0.0f;
    dir.z = path->pos.z - owner->pos.z;
    dist = dir.normalize_length();
    if (dist == 0.0f)
        dir.zero();
    while (dir.z * path->dir.z + dir.y * path->dir.y + dir.x * path->dir.x < 0.0f) {
        path_start = path->pos;
        path = ai_path_pop_node(path);
        if (!path)
            return false;
        dir.x = path->pos.x - owner->pos.x;
        dir.y = 0.0f;
        dir.z = path->pos.z - owner->pos.z;
        dist = dir.normalize_length();
        if (dist == 0.0f)
            dir.zero();
    }
    first_dist = dist;
    if (path) {
        dist += path->cost;
        if (!(dist > 0.0f)) {
            ai_path_free(path);
            path = NULL;
            return false;
        }
    }

    {
        float stop = owner->stopping_distance(NULL);
        float clear = stop + owner->info->head_height + 2.0f;
        float reach, brake, target_speed, factor, facing_speed;
        ai_path *n;

        for (n = path; n && clear > 0.0f; n = n->next) {
            if (pf_check_direct_walk(&owner->pos, &n->pos, owner->layer, owner->info->size, &clear)) {
                ai_path_free(path);
                path = NULL;
                return false;
            }
            {
                vector3 dn = owner->pos - n->pos;
                clear = clear - (float)sqrt(dn.x * dn.x + dn.y * dn.y + dn.z * dn.z);
            }
        }
        reach = speed < 0.001f ? stop : owner->distance_to_reach_speed(speed);
        target_speed = owner->speed_scale;
        brake = (owner->speed_scale * 2.0f) * (owner->speed_scale * 2.0f)
                / (owner->slowdown_rate * owner->speed_scale * 2.0f);
        if (speed < 0.0001f) {
            if (dist <= reach)
                target_speed = 0.0f;
            else if (dist <= brake)
                target_speed = owner->speed_scale;
        } else {
            if (dist <= reach)
                target_speed = speed;
            else if (dist <= brake)
                target_speed = (speed + owner->speed_scale) * 0.5f;
        }
        factor = 1.0f;
        if (path && path->next) {
            vector3 nd = path->next->pos - owner->pos;
            nd.normalize();
            factor = ai_turn_speed_factors[(int)((nd.x * dir.x + nd.z * dir.z + nd.y * dir.y) * 5.0f + 5.5f)];
        }
        factor *= owner->speed_scale;
        if (factor < owner->forward_speed) {
            if (owner->distance_to_reach_blend(factor / owner->run_speed) > first_dist)
                target_speed = 0.0f;
        }
        facing_speed = (dir.y * owner->orient.fvec.y + dir.z * owner->orient.fvec.z + dir.x * owner->orient.fvec.x
                        + 1.0f) * 0.5f * owner->speed_scale;
        if (facing_speed < target_speed)
            target_speed = facing_speed;
        if (path_flags & 1)
            target_speed = 0.0f;
        if (owner->speed_scale > 0.0f && target_speed > owner->speed_scale)
            target_speed = owner->speed_scale;
        owner->set_6ac(target_speed);
    }
    return true;
}

// 0x4b3c80  Confidence: likely. Sub-mode AIS_TRAVEL_STEER: turns at twice the moving turn rate (the
// char_info value itself is doubled and halved again); single player only, the party members walking
// ahead of the owner within 3 units step aside (sub-mode 0x2b), and the speed is capped by the slowest
// awake party member (0.8 of it while one of them is not marked 0x8000000).
void ai_state::travel_steer_update()
{
    profile_label label("ai_travel_steer_update");
    bool turning;
    bool slow;
    float max_speed;
    Character *o;
    float dist, speed;

    owner->info->moving_turn_rate = owner->info->moving_turn_rate + owner->info->moving_turn_rate;
    turning = turn_towards(&travel_dest, false);
    owner->info->moving_turn_rate = owner->info->moving_turn_rate * 0.5f;
    if (!(game_flags & 2)) {
        Character *c;
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (c->type == 7 && c->ai.is_mode(AI_STATE_TRAVELING, AIS_TRAVEL_PATH))
                c->ai.goals_remove_type9();
    }
    if (turning)
        return;
    slow = false;
    max_speed = 99.9f;
    if (!(game_flags & 2)) {
        Player *p;
        for (p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (!c || c == owner || (p->flags & 4))
                continue;
            if (!(c->flags & CF_SUMMON)) {
                if (c->speed_scale < max_speed && !c->has_status(0xd))
                    max_speed = c->speed_scale;
                if (!(c->ai.flags & 0x8000000))
                    slow = true;
            }
            if (!c->ai.is_mode(AI_STATE_TRAVELING, -1))
                continue;
            {
                vector3 d = c->pos - owner->pos - owner->centre_offset;
                float len = d.normalize_safe_length();
                if (d.z * owner->orient.fvec.z + d.y * owner->orient.fvec.y + d.x * owner->orient.fvec.x > 0.5f
                    && len < 3.0f) {
                    vector3 dest;
                    int layer;
                    if (d.z * owner->orient.fvec.x - d.x * owner->orient.fvec.z < 0.0f)
                        d = owner->orient.rvec + owner->orient.fvec;
                    else
                        d = owner->orient.fvec - owner->orient.rvec;
                    dest = c->pos + d * 3.0f;
                    layer = pf_trace_edge_crossings(&c->pos, c->layer, dest);
                    pf_find_nearest_navigable_point(&dest, c->info->size, layer);
                    c->ai.path_find_new(&c->pos, c->layer, &dest, layer, c->info->size, 0.0f);
                    c->ai_add_goal9(&dest, layer, 5000, true);
                    c->ai.set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_ASIDE);
                }
            }
        }
    }
    o = owner;
    {
        float dx = travel_dest.x - o->pos.x;
        float dz = travel_dest.z - o->pos.z;
        dist = (float)sqrt(dz * dz + dx * dx);
    }
    speed = dist * o->speed_scale;
    if (dist < 0.25f)
        speed = 0.0f;
    else if (speed > o->speed_scale)
        speed = o->speed_scale;
    if (slow == true)
        max_speed *= 0.8f;
    if (speed > max_speed)
        speed = max_speed;
    if (o->speed_scale > 0.0f && speed > o->speed_scale)
        speed = o->speed_scale;
    o->set_6ac(speed);
}

// ---- the mode handlers -------------------------------------------------------------------------------
// 0x4b4100  Confidence: likely (byte-indexed jump table 0x4b4564 into 0x4b4544 over sub-modes 0..0x2b).
void ai_state::mode_traveling_frame()
{
    profile_label label("ai_mode_traveling_frame");
    Character *mount;
    Character *leader;
    float s;

    if (owner->handle == world_mouse_target_handle)
        ai_mode_frame_counts[AI_STATE_TRAVELING]++;
    mount = owner->get_mount();
    if (mount && mount->ai.is_mode(AI_STATE_TRAVELING, AIS_TRAVEL_PATH))
        return;
    switch (submode) {
    case AIS_TRAVEL_PATH:
        if (follow_path(0.0f))
            return;
        break;
    case AIS_TRAVEL_STEER:
        travel_steer_update();
        return;
    case AIS_TRAVEL_TURN:
        ai_move_flags = 6;
        if (turn_towards(&travel_dest, false))
            return;
        break;
    case AIS_TRAVEL_FOLLOW:
    case AIS_TRAVEL_FOLLOW_SLOW:
        if (!group)
            break;
        leader = living_from_handle(group->leader);
        if (!leader)
            break;
        s = leader->get_speed();
        if (submode == AIS_TRAVEL_FOLLOW_SLOW)
            s *= 0.5f;
        if (follow_path(s))
            return;
        if (owner->get_698() < 0.0001f) {
            set_mode(AI_STATE_TRAVELING, AIS_TRAVEL_WAIT);
            return;
        }
        travel_dest = owner->pos + leader->orient.fvec;
        turn_towards(&travel_dest, false);
        s = min(s, owner->speed_scale);
        owner->set_6ac(s);
        return;
    case AIS_TRAVEL_WAIT:
        if (!group)
            break;
        leader = living_from_handle(group->leader);
        if (!leader)
            break;
        if (owner->type == 7 && (player_from_handle(owner->handle)->flags & 4))
            break;
        if (owner->get_698() < 0.0001f) {
            travel_dest = leader->get_center();
            turn_towards(&travel_dest, false);
        }
        owner->set_6ac(0.0f);
        return;
    case AIS_TRAVEL_GOTO:
    case AIS_TRAVEL_ESCORT:
    case AIS_TRAVEL_ASIDE:
        follow_path(0.0f);
        return;
    case AIS_TRAVEL_WANDER:
        if (timer40.valid() && timer40.elapsed()) {
            vector3 p;
            int i = 0;
            for (;;) {
                float r = frand() * wander_radius + 0.1f;
                if (2.0f > r)
                    r = 2.0f;
                p.random_direction();
                p.x *= r;
                p.y = 0.0f;
                p.z *= r;
                p += cast_point;
                if (pf_point_is_navigable(&p, owner->info->size, owner->layer) == true)
                    break;
                if (++i >= 10)
                    goto idle;
            }
            path_to(&owner->pos, owner->layer, &p, owner->layer, 0, owner->info->size, false, true);
            if ((game_flags & 2) && character_is_locally_controlled(owner))
                mp_send_character_walk(owner, owner->speed_scale == owner->walk_speed);
            timer40.invalidate();
        }
        if (follow_path(0.0f))
            return;
        owner->set_6ac(0.0f);
        if (!timer40.valid())
            timer40.SetRandomTimeout(15000, 20000);
        return;
    }
idle:
    go_idle();
}

// 0x4b4590  Confidence: exact (dec chain over 0x27..0x29).
void ai_state::mode_cast_spell_frame()
{
    profile_label label("ai_mode_cast_spell_frame");

    if (owner->handle == world_mouse_target_handle)
        ai_mode_frame_counts[AI_STATE_CAST_SPELL]++;
    switch (submode) {
    case AIS_CAST_SPELL:
        break;
    case AIS_CAST_AT_TARGET:
        if (emerge_move == -1) {
            Character *t = living_from_handle(target);
            if (!t) {
                set_mode(prev_mode, prev_submode);
                return;
            }
            if (target != owner->handle && turn_towards(&t->pos, false) == true)
                return;
        }
        break;
    case AIS_CAST_AT_POINT:
        if (turn_towards(&cast_point, false) == true)
            return;
        break;
    default:
        return;
    }
    cast_spell_update();
}

// 0x4b4660  Confidence: exact (a switch on the species, the cases in the order of their spells 0x68..0x6b; jump table 0x4b48f4 / 0x4b4908 for
// the four emerging bosses 0xb5, 0xb6, 0xc2, 0xc3). A summon's emerge move: species 0x90 takes its weapon
// at trigger 7, species 0xce shows its prop at trigger 2 and drops the prop tag at trigger 3; a boss
// emerging from its effect (owner flag 0x800) casts its entrance spell once the effect is over. When the
// move ends (or trigger 6) the avoidance area of the summon effect is removed and the AI goes idle.
void ai_state::summon_emerge_update()
{
    int species = owner->info->species;

    if (species == 0x90) {
        if (owner->move_trigger_fired(7, 1) == 1 && !owner->equipment[5]) {
            item_info *old1, *old2;
            item_page_in(owner->info->default_weapons[0], 8, 0);
            character_equip_item(&old2, &old1, owner, 5, item_create_weapon(owner->info->default_weapons[0]));
        }
    } else if (species == 0xce) {
        if (owner->move_trigger_fired(2, 1) == 1)
            owner->attachments[0].slot = 1;
        if (owner->move_trigger_fired(3, 1) == 1)
            owner->prop_tag = 0;
    } else if (owner->flags & 0x800) {
        effect_object *fx = (effect_object *)obj_from_handle(owner->move_fx);
        if (!fx || fx->instance->get_time_left() < 0.0001f) {
            Character *o;
            owner->obj_flags &= ~1;
            owner->obj_flags |= 4;
            o = owner;
            switch (o->info->species) {
            case 0xc2:
                spell_cast_execute(o, 0x68, -1, &o->pos, o->layer, 0, 0, 0);
                return;
            case 0xb5:
                spell_cast_execute(o, 0x69, -1, &o->pos, o->layer, 0, 0, 0);
                return;
            case 0xb6:
                spell_cast_execute(o, 0x6a, -1, &o->pos, o->layer, 0, 0, 0);
                return;
            case 0xc3:
                spell_cast_execute(o, 0x6b, -1, &o->pos, o->layer, 0, 0, 0);
                return;
            }
        }
        return;
    }
    if (owner->move_finished(emerge_move) || owner->move_trigger_fired(6, 1) == 1) {
        effect_object *fx;
        for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
            if (fx->kind == 8 && fx->avoid_area >= 0) {
                printf("Destroying AA after emerge anim is over\n");
                avoid_area_remove(fx->avoid_area);
                fx->avoid_area = -1;
                break;
            }
        }
        owner->flags &= ~CF_SUMMONLIKE;
        set_mode(AI_STATE_IDLE, -1);
    }
    ai_move_flags |= 4;
}

// 0x4b4920  Confidence: exact (VC6 byte match, m-1020b w8, D13202: the summon test before the profile label, the
// velocity assigned to a local (operator=), both failure tails written out: the binary cross-jumps them). A summon spell in a network game needs the summon claim first.
void ai_state::cast_spell_update()
{
    int summon = summon_find_charinfo(mode_target) >= 0;
    profile_label label("ai_cast_spell_update");

    if ((game_flags & 2) && summon && !mp_summon_try_claim())
        return;
    ai_move_flags |= 4;
    owner->set_6ac(0.0f);
    vector3 vel;
    vel = owner->instance->sum_root_velocity();
    if (vel.len() > 0.1f)
        return;
    if (emerge_move < 0) {
        bool flag = summon != 0;
        emerge_move = owner->perform_spell_cast(mode_target, flag);
        if (emerge_move == -1) {
            target = -1;
            set_mode(prev_mode, prev_submode);
            return;
        }
        if (game_flags & 2)
            mp_send_cast_anim(owner, mode_target, flag);
        _unk88 = mode_target;
        flags |= 0x40000002;
    }
    if (!owner->move_playing(emerge_move, NULL)) {
        flags &= ~2;
        target = -1;
        set_mode(prev_mode, prev_submode);
        return;
    }
    if (owner->move_trigger_fired(7, 0))
        spell_cast_execute(owner, mode_target, target, &cast_point, cast_arg, 0, 0, 1);
}

// 0x4b4ad0  Confidence: exact (jump table 0x4b4d5c over sub-modes 0x2c..0x31). Knocked down (0x2c: fall,
// 0x2d: lie for disable_ms, 0x2e: get up), pushed (0x2f), stunned (0x30, then status 10 for _unk48 ms)
// and the timed disable 0x31 (10 s).
void ai_state::mode_disabled_frame()
{
    move_info *m;

    switch (submode) {
    case AIS_KNOCKED_DOWN:
        if (!disable_timer.valid()) {
            m = owner->find_move(9, MOVE_KNOCKED_DOWN);
            if (!m)
                break;
            emerge_move = owner->play_move((int)m, 1.0f, true, true, false);
            disable_timer.SetTimeout(0);
            if (emerge_move == -1)
                go_idle();
            return;
        }
        if (owner->move_trigger_fired(6, 0))
            set_mode(AI_STATE_DISABLED, AIS_KNOCKED_DOWN_HOLD);
        return;
    case AIS_KNOCKED_DOWN_HOLD:
        if (timer.GetElapsedTime() > disable_ms) {
            set_mode(AI_STATE_DISABLED, AIS_GET_UP);
            disable_timer.invalidate();
        }
        return;
    case AIS_GET_UP:
        if (!disable_timer.valid()) {
            m = owner->find_move(9, MOVE_GET_UP);
            if (!m)
                break;
            emerge_move = owner->play_move((int)m, 1.0f, true, true, false);
            disable_timer.SetTimeout(0);
            if (emerge_move == -1)
                go_idle();
            return;
        }
        if (owner->move_trigger_fired(6, 0))
            go_idle();
        return;
    case AIS_PUSHED:
        if (!disable_timer.valid()) {
            m = owner->find_move(9, MOVE_STUNNED);
            if (!m)
                break;
            emerge_move = owner->play_move((int)m, 1.0f, true, true, false);
            disable_timer.SetTimeout(0);
            if (emerge_move == -1)
                go_idle();
            return;
        }
        if (owner->move_finished(emerge_move) == true)
            set_mode(prev_mode, prev_submode);
        return;
    case AIS_STUNNED:
        if (!disable_timer.valid()) {
            m = owner->find_move(9, MOVE_STUNNED);
            if (m) {
                emerge_move = owner->play_move((int)m, 1.0f, true, true, false);
                disable_timer.SetTimeout(0);
                if (emerge_move != -1)
                    return;
            }
            disable_31();
            return;
        }
        if (owner->move_finished(emerge_move) == true) {
            set_mode(prev_mode, prev_submode);
            owner->status_effect_add(10, _unk48, -1, 0.0f, -1, false);
        }
        return;
    case AIS_DISABLED_TIMED:
        if (timer.GetElapsedTime() > 10000) {
            owner->clear_flag_2_4();
            break;
        }
        return;
    default:
        return;
    }
    go_idle();
}

// 0x4b4d80  Confidence: exact (t stays on the FPU stack across the switch; t = 1 before the flag). The stance blend fades
// over 500 ms; the instance alpha goes from 255 (visible) to 127 (hidden).
void ai_state::mode_hiding_frame()
{
    float t = 0.0f;

    switch (submode) {
    case AIS_ENTER_HIDING:
        t = timer.GetElapsedTime() * 0.002f;
        if (t >= 1.0f) {
            t = 1.0f;
            set_mode(AI_STATE_HIDING, AIS_HIDING);
        }
        owner->stance_blend = 1.0f - t;
        break;
    case AIS_HIDING:
        t = 1.0f;
        break;
    case AIS_LEAVE_HIDING:
        t = timer.GetElapsedTime() * 0.002f;
        if (t >= 1.0f) {
            t = 1.0f;
            flags &= ~2;
            set_mode(pending_mode, pending_submode);
            goal_remove(goal_find(1, AI_GOAL_HIDE, 0, 0, -1, 0, 0, AI_NO_SPELL));
        }
        owner->stance_blend = t;
        break;
    }
    if (submode == AIS_LEAVE_HIDING || mode != AI_STATE_HIDING)
        t = 1.0f - t;
    owner->instance->_unkb4 = (ubyte)(int)(255.0f - t * 128.0f);
}

// 0x4b4eb0  Confidence: exact (VC6 byte match). Entering plays the sneak-in move and (single player) holds the party;
// the sneak lasts the hide / sneak skill level times 10 s (timer44); leaving plays the sneak-out move,
// starts the skill's cooldown and releases the party hold again.
void ai_state::mode_sneaking_frame()
{
    float t = 0.0f;
    int sub = submode;

    if (mode == AI_STATE_SNEAKING) {
        if (!disable_timer.valid()) {
            int id;
            if (submode == AIS_ENTER_SNEAKING) {
                timer44.invalidate();
                owner->flags |= 0x400;
                id = MOVE_SNEAK_IN;
                if ((game_flags & 1) && !(game_flags & 0x1000))
                    party_toggle_hold();
                else
                    flags |= 0x4000000;
            } else {
                id = MOVE_SNEAK_OUT;
            }
            disable_timer.SetTimeout(0);
            emerge_move = -1;
            move_info *m = owner->find_move(10, id);
            emerge_move = owner->play_move((int)m, 1.0f, false, false, false);
            if (emerge_move == -1)
                return;
            flags = (flags & 0x7ffffffd) | 2;
            if (submode == AIS_ENTER_SNEAKING)
                owner->stance_blend = 1.0f;
            else
                owner->stance_blend = 0.0f;
        }
        if (emerge_move >= 0) {
            t = owner->move_progress(emerge_move);
            if (t >= 0.5f && !(flags & 0x80000000)) {
                flags |= 0x80000000;
                if (submode == AIS_ENTER_SNEAKING)
                    owner->stance_blend = 0.0f;
                else
                    owner->stance_blend = 1.0f;
            } else if (owner->move_finished(emerge_move)) {
                flags &= ~2;
                if (submode == AIS_ENTER_SNEAKING) {
                    timer44.SetTimeout(owner->skill[SKILL_HIDE_SNEAK] * 10000);
                    go_idle();
                } else {
                    owner->flags &= ~0x400;
                    if (owner->type == 7)
                        player_start_skill_cooldown(owner, SKILL_HIDE_SNEAK);
                    if (timer44.valid() == true)
                        set_mode(pending_mode, pending_submode);
                    else
                        go_idle();
                    timer44.invalidate();
                    disable_timer.invalidate();
                    owner->update_locomotion_anim();
                    owner->set_speed_scale(owner->run_speed, false);
                    if ((game_flags & 1) && !(flags & 0x4000000))
                        party_toggle_hold();
                }
            }
        }
    } else {
        t = 1.0f;
    }
    if (sub == AIS_LEAVE_SNEAKING)
        t = 1.0f - t;
    owner->instance->_unkb4 = (ubyte)(int)(255.0f - t * 128.0f);
}
