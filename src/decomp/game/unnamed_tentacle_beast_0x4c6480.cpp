// unnamed_tentacle_beast_0x4c6480.cpp - Sum.exe v1.40, code 0x4c6480-0x4c8020: the tentacle beast of the
// sewers (species 0x93): four tentacles, each a character with its own AI, that rise, swipe, squeeze or grab
// and throw party members, and dive. File name NOT known: a placeholder (src/decomp/README.md). No assert
// path, no strings.tbl block; cut by its static initialiser run (entries 329-332, 0x588528-0x588534) and its
// data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_tentacle_records (0x4c6480) to cmd_tentacle_test (0x4c7e90-0x4c8020). The Giant
// Salamanka unit opens with its initialiser 0x4c8030 (entry 333).
// Data: .data 0x59dde8-0x59debb (the four mesh tag names, "splash.vfx", the two command names and help
// texts, "$npc034"); .bss 0x248c320-0x248c7ad (tentacle_move_2 first, tentacle_test_mode last; the Giant
// Salamanka's ccmd_gsal_combat is at 0x248c7b0). 0x248c324 and 0x248c784 are never accessed.
// The thiscall functions run on a tentacle's AI record; tentacle_beast_active_tentacle is that tentacle's
// record (tentacle_find), set by the load and the frame. Sub-modes: 0xf attack, 0x16 back to 0xf, 0x35 the
// grab, 0x37 the release. Return codes of tentacle_beast_ai_select_action: 3 a move and target were chosen,
// 4 nothing in reach, 5 busy.

#include <math.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/effects/fx.h"
#include "engine/camera/camera.h"
#include "engine/rendering/levelrender.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define AIS_CLOSE_COMBAT    0x0f      // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE       0x16
#define AIS_TENTACLE_GRAB   0x35      // names ours
#define AIS_TENTACLE_RELEASE 0x37

#define TENTACLE_SUBMERGED  0x1
#define TENTACLE_RISING     0x2
#define TENTACLE_SINKING    0x4

#define CF_TENTACLE_HELD    0x1028    // Character +0x320 bits of a held victim (CF_HIDDEN | 0x20 | 0x1000)

// ---- .bss (address order) -----------------------------------------------------------------------------
move_info *tentacle_move_2;           // 0x248c320 find_move(0, 2): the squeeze
int  tentacle_248c324;                // 0x248c324 never accessed
// 0x4c6480  Confidence: exact. static_init_tentacle_records: "jmp 0x4c6490" (initialiser entry 329).
// 0x4c6490  Confidence: likely. init_tentacle_records: the compiler's constructor of the four records
// (vector_constructor_iterator over the five slots, then the GameTimers +0xa8, +0xc0, +0xc4, +0xc8, +0x100).
tentacle_info tentacle_beast_tentacles[NUM_TENTACLES];   // 0x248c328
// 0x4c6500  Confidence: exact. static_init_tentacle_beast_timer: "jmp 0x4c6510" (entry 330).
// 0x4c6510  Confidence: exact. init_tentacle_beast_timer (a tail jump to the GameTimer constructor).
GameTimer tentacle_beast_grab_timer;  // 0x248c768
bool tentacle_beast_battle_started;   // 0x248c76c
int  tentacle_beast_grab_ready;       // 0x248c770
int  tentacle_beast_tentacles_lost;   // 0x248c774
// ccmd_tentacle_test 0x248c778 and ccmd_tentacles 0x248c788 are defined with their initialisers below.
int  tentacle_248c784;                // 0x248c784 never accessed
move_info *tentacle_move_1;           // 0x248c794 find_move(0, 1): the swipe
int  tentacle_beast_tentacles_up;     // 0x248c798
int  tentacle_last_cycled;            // 0x248c79c tentacle_beast_cycle_tentacles
move_info *tentacle_move_87;          // 0x248c7a0 find_move(0, 0x87): the release (looked up again there)
move_info *tentacle_move_86;          // 0x248c7a4 find_move(0, 0x86): the grab
tentacle_info *tentacle_beast_active_tentacle;   // 0x248c7a8
bool tentacle_beast_loaded;           // 0x248c7ac
bool tentacle_test_mode;              // 0x248c7ad

// 0x4c64f0  Confidence: exact. The GameTimer member is constructed; nothing else.
tentacle_slot::tentacle_slot()
{
}

#ifdef EXPERIMENTAL_BUILD
// EXPERIMENTAL_BUILD: tentacle TentacleGrab
// The grab and throw (G250, docs/future/tentacle-grab.md section 7.3; the same behaviour as SummonerFix's
// tentacle.c, which was tested in game). Off by default: [Experimental] TentacleGrab=1 turns it on,
// TentacleGrabChance (percent per decision, default 10) sets how often a tentacle that may grab tries to, [Debug]
// TraceTentacle=1 logs the details (misses, hold ticks) as well. With the switch at 0 every function below returns
// to the original code at once, so the game runs as the fixed build.
// The changes, by the slip numbers of the plan (section 5):
// - select (0x4c6520): the original plus a grab branch before the ladder (grab_ready, single player, the chance,
//   a candidate with "flail" (14, 0x8c) and "throwndown" (8, 1), nearest to the live pickup point or in reach);
//   a watchdog re-arms the grab timer when the gate is stuck closed (slips 1-4).
// - grab (0x4c6850): trigger 7 only when move_trigger_fired returns 1 (slip 8); the victim alive, not held, with
//   the flail pose, within reach + 1 (slip 5); release_hp from the tentacle's HP (slip 7); the "pick up ready"
//   lookup NULL-tested (slip 10); the lead moves only off a leading victim (slip 9); a whiff re-arms the gate
//   (slip 4).
// - hold tick (in tentacle_beast_ai_frame): 3 damage every 6 s instead of a 3 HP heal (slip 6).
// - release (0x4c6a80): trigger 8 tested for 1; a gone or dead victim is not thrown (slip 8).
// - battle end (tentacle_begin_sink 0x4c6c50): a held or flying victim is put down, then the tentacle sinks
//   (slip 12).
// - frame hook (0x4c7960): a gone / dead held or thrown victim is put down at once (slip 11); a landing off the
//   navigation mesh is moved onto it; a fall 3.0 below the start and the aim (water) or 3 s of flight lands it
//   (slip 13).
// - tentacle_collect_area_hits (0x4c77c0): at most four hits (G251, slip 15).
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "experimental/experimental.h"
#include "engine/ai/pathfinding.h"

#define EXP_TENTACLE_GRAB_COOLDOWN 10000      // ms, as the landing arms it (0x4c7c28)
#define EXP_TENTACLE_FLIGHT_TIMEOUT 3.0f      // s
#define EXP_TENTACLE_FALL_LIMIT 3.0f          // below both the throw's start and its aim point

static int   exp_tentacle_state = -1;         // -1 not read yet, 0 off, 1 on
static float exp_tentacle_chance;             // 0..1
static int   exp_tentacle_trace_on;
// Per record: the throw's start height and its flight time (the fall limit and the timeout).
static float exp_tentacle_throw_start_y[NUM_TENTACLES];
static float exp_tentacle_flight_time[NUM_TENTACLES];

// The switches, read once.
static bool exp_tentacle_on()
{
    int chance;

    if (exp_tentacle_state < 0) {
        exp_tentacle_state = exp_ini_int("Experimental", "TentacleGrab", 0) != 0;
        chance = exp_ini_int("Experimental", "TentacleGrabChance", 10);
        if (chance < 0)
            chance = 0;
        if (chance > 100)
            chance = 100;
        exp_tentacle_chance = chance / 100.0f;
        exp_tentacle_trace_on = exp_ini_int("Debug", "TraceTentacle", 0) != 0;
        if (exp_tentacle_state)
            exp_log("tentacle grab on: chance %d%% per decision while a grab is allowed%s", chance,
                    exp_tentacle_trace_on ? ", tracing" : "");
        else
            exp_log("tentacle grab off ([Experimental] TentacleGrab=0)");
    }
    return exp_tentacle_state == 1;
}

// A detail line ([Debug] TraceTentacle=1).
static void exp_tentacle_trace(const char *fmt, ...)
{
    char line[512];
    va_list ap;

    if (!exp_tentacle_trace_on)
        return;
    va_start(ap, fmt);
    _vsnprintf(line, sizeof line - 1, fmt, ap);
    va_end(ap);
    line[sizeof line - 1] = 0;
    exp_log("%s", line);
}

static int exp_tentacle_index(tentacle_info *t)
{
    return (int)(t - tentacle_beast_tentacles);
}

static bool exp_tentacle_index_ok(int i)
{
    return i >= 0 && i < NUM_TENTACLES;
}

static void exp_tentacle_arm_grab_timer()
{
    tentacle_beast_grab_timer.SetTimeout(EXP_TENTACLE_GRAB_COOLDOWN);
}

// Dead (0x80) or at 0 HP. A held victim always has 0x8 (the grab sets it), so 0x8 cannot tell dying from held.
static bool exp_tentacle_is_down(Character *c)
{
    return (c->flags & CF_DEAD) || c->hp <= 0.0f;
}

// Alive, not held, with the "flail" pose and the "throwndown" move.
static bool exp_tentacle_grabbable(Character *c)
{
    move_info *flail;

    if (c == NULL || (c->flags & (CF_HIDDEN | CF_DEAD)))
        return false;
    flail = charinfo_find_move(c->info, 0xe, 0x8c, true);
    if (flail == NULL || flail->anim == -1)
        return false;
    return c->find_move(8, 1) != NULL;
}

// The tentacle's ready / ready-hurt anims, each lookup NULL-tested (the original did not).
static void exp_tentacle_set_anims(Character *tentacle, int ready_id, int hurt_id)
{
    move_info *ready = charinfo_find_move(tentacle->info, 0xe, ready_id, true);
    move_info *hurt = charinfo_find_move(tentacle->info, 0xe, hurt_id, true);

    if (ready)
        tentacle->anim_ready = ready->anim;
    if (hurt)
        tentacle->anim_ready_hurt = hurt->anim;
    tentacle->update_locomotion_anim();
}

// Faces level, as the landing does.
static void exp_tentacle_face_level(Character *c)
{
    vector3 fwd;
    vector3 up(0.0f, 1.0f, 0.0f);

    fwd = c->orient.fvec;
    fwd.y = 0.0f;
    c->orient.set_from_forward_up(fwd, up);
}

// To the nearest navigable point when off the navigation mesh (as luminar_frame_hook does). A body that fell below
// lift_y (into the water) is lifted back to it, since character_find_ground looks for a floor from above.
static void exp_tentacle_snap_to_nav(Character *c, float lift_y)
{
    if (pf_point_is_navigable(&c->pos, c->info->size, c->layer))
        return;
    pf_find_nearest_navigable_point(&c->pos, c->info->size, c->layer);
    if (c->pos.y < lift_y)
        c->pos.y = lift_y;
}

// The landing of tentacle_beast_frame_hook with the fixes: no go_idle and no "throwndown" for a dead or dying
// body, and the spot moved onto the navigation mesh. A body at 0 HP that is not dead yet keeps 0x8 (dying).
static void exp_tentacle_put_down(Character *c, float lift_y)
{
    bool alive = !exp_tentacle_is_down(c);
    bool dying = !alive && !(c->flags & CF_DEAD);
    move_info *m;

    c->flags &= ~(dying ? (CF_TENTACLE_HELD & ~CF_HIDDEN) : CF_TENTACLE_HELD);
    if (alive)
        c->ai.go_idle();
    exp_tentacle_face_level(c);
    exp_tentacle_snap_to_nav(c, lift_y);
    c->find_ground();
    m = alive ? c->find_move(8, 1) : NULL;
    if (m)
        c->set_flag_2_4(c->play_move((int)m, 1.0f, false, false, false));
    else
        c->clear_flag_2_4();
}

// Puts a held or thrown victim down now and clears the record (battle end, victim gone or dead).
static void exp_tentacle_drop(tentacle_info *t, const char *why)
{
    int idx = exp_tentacle_index(t);
    int handles[2];
    Character *c;
    float lift_y;
    int i;

    handles[0] = t->victim;
    handles[1] = t->thrown;
    t->victim = -1;
    t->thrown = -1;
    for (i = 0; i < 2; i++) {
        if (handles[i] == -1)
            continue;
        c = living_from_handle(handles[i]);
        exp_log("tentacle %d: drop %s victim 0x%x (%s), HP %.1f", idx, i ? "thrown" : "held", handles[i], why,
                c ? c->hp : 0.0f);
        if (c == NULL)
            continue;
        lift_y = c->pos.y;
        if (i == 1 && exp_tentacle_index_ok(idx) && exp_tentacle_throw_start_y[idx] > lift_y)
            lift_y = exp_tentacle_throw_start_y[idx];
        exp_tentacle_put_down(c, lift_y);
    }
    t->squeeze_timer.invalidate();
    if (t->character)
        exp_tentacle_set_anims(t->character, 0x93, 0x95);
    exp_tentacle_arm_grab_timer();
}

// ---- select ----

// "$tag-26-pickup" in the tentacle's current pose; the load-time point (pickup_pos) without the tag.
static void exp_tentacle_pickup_point(Character *tentacle, tentacle_info *t, vector3 *out)
{
    matrix orient;
    int tag = tentacle->instance->character->find_mesh_tag("$tag-26-pickup");

    if (tag == -1) {
        *out = t->pickup_pos;
        return;
    }
    tentacle->instance->get_bone_world_transform(tag, &tentacle->orient, &tentacle->pos, &orient, out);
}

// Distance on X/Z (y 0 on both, as the squeeze search does).
static float exp_tentacle_flat_distance(const vector3 *a, const vector3 *b)
{
    vector3 p = *a;
    vector3 q = *b;

    p.y = 0.0f;
    q.y = 0.0f;
    return distance(&p, &q);
}

// The nearest grabbable candidate within 3.0 of the pickup point, else the first within reach.
static Character *exp_tentacle_pick_target(Character *tentacle, tentacle_info *t, Character **list, int count)
{
    vector3 pickup;
    Character *best = NULL;
    float best_d = 3.0f;
    float d;
    int i;

    exp_tentacle_pickup_point(tentacle, t, &pickup);
    for (i = 0; i < count; i++) {
        if (!exp_tentacle_grabbable(list[i]))
            continue;
        d = exp_tentacle_flat_distance(&list[i]->pos, &pickup);
        if (d < best_d) {
            best_d = d;
            best = list[i];
        }
    }
    if (best)
        return best;
    for (i = 0; i < count; i++) {
        if (exp_tentacle_grabbable(list[i]) && tentacle->distance_between_edges(list[i], true, -1) < t->reach)
            return list[i];
    }
    return NULL;
}

// Slip 4 watchdog: a grab that never reached its end (the tentacle sank or was reset during the grab move) leaves
// grab_ready 0 with the grab timer invalid. When nothing is held, thrown or being grabbed, the timer is armed.
static void exp_tentacle_gate_watchdog()
{
    tentacle_info *t;
    int i;

    if (tentacle_beast_grab_ready || tentacle_beast_grab_timer.valid())
        return;
    for (i = 0; i < NUM_TENTACLES; i++) {
        t = &tentacle_beast_tentacles[i];
        if (t->victim != -1 || t->thrown != -1)
            return;
        if (t->character && t->character->ai.is_mode(AI_STATE_COMBAT, AIS_TENTACLE_GRAB))
            return;
    }
    exp_log("tentacle: grab gate was stuck closed, timer armed");
    exp_tentacle_arm_grab_timer();
}

// The grab decision, before the original ladder: the target or NULL.
static Character *exp_tentacle_try_grab(Character *tentacle, tentacle_info *t, Character **list, int count)
{
    Character *target;

    exp_tentacle_gate_watchdog();
    if (!tentacle_beast_grab_ready || (game_flags & 2) || tentacle_move_86 == NULL)
        return NULL;
    if (t->victim != -1 || t->thrown != -1)
        return NULL;
    if (!(frand() < exp_tentacle_chance))
        return NULL;
    target = exp_tentacle_pick_target(tentacle, t, list, count);
    if (target == NULL)
        exp_tentacle_trace("tentacle %d: grab roll passed, no grabbable candidate", exp_tentacle_index(t));
    return target;
}

// tentacle_beast_ai_select_action with the grab branch.
static int exp_tentacle_select_action(ai_state *ai)
{
    Character *owner = ai->owner;
    tentacle_info *t = tentacle_beast_active_tentacle;
    Character *list[5];
    Character *c;
    Character *grab;
    move_info *move;
    float best;
    float d;
    float r;
    int count;
    int i;
    int j;
    bool seen;

    if (owner->combat.is_busy() == true)
        return 5;
    if (owner->move_playing(ai->emerge_move, NULL))
        return 5;
    t->move = NULL;
    if (!t->think_timer.valid())
        t->think_timer.SetRandomTimeout(2500, 5000);
    if (!t->think_timer.elapsed())
        return 5;
    t->think_timer.invalidate();
    count = 0;
    for (i = 0; i < 5; i++) {
        if (t->slots[i].handle == -1)
            continue;
        c = living_from_handle(t->slots[i].handle);
        if (c && !(c->flags & CF_DEAD)) {
            list[0] = c;
            count = 1;
            break;
        }
    }
    for (c = living_list.live_next; c != &living_list && count < 5; c = c->live_next) {
        if (c->type != 7 || (c->flags & (CF_HIDDEN | CF_DEAD)))
            continue;
        seen = false;
        for (j = 0; j < count; j++) {
            if (list[j] == c)
                seen = true;
        }
        if (!seen)
            list[count++] = c;
    }
    if (count == 0)
        return 4;

    grab = exp_tentacle_try_grab(owner, t, list, count);
    if (grab) {
        t->move = tentacle_move_86;
        t->target = grab->handle;
        tentacle_beast_grab_ready = 0;
        exp_log("tentacle %d: grab pick, victim 0x%x, HP %.1f", exp_tentacle_index(t), grab->handle, grab->hp);
        return 3;
    }

    // The original ladder. Its grab rung (< 0.0) never fires and would fall into the swipe search.
    r = frand();
    if (r < 0.0f)
        move = tentacle_move_86;
    else if (r < 0.3f)
        move = tentacle_move_2;
    else
        move = tentacle_move_1;
    t->move = NULL;
    if (move == tentacle_move_2) {
        best = 1000.0f;
        for (i = 0; i < count; i++) {
            d = exp_tentacle_flat_distance(&list[i]->pos, &t->squeeze_pos);
            if (d < 3.0f && d < best) {
                best = d;
                t->move = tentacle_move_2;
                t->target = list[i]->handle;
            }
        }
        if (t->move)
            return 3;
    }
    for (i = 0; i < count; i++) {
        if (owner->distance_between_edges(list[i], true, -1) < t->reach) {
            t->move = tentacle_move_1;
            t->target = list[i]->handle;
            return 3;
        }
    }
    return 4;
}

// ---- grab ----

// Slip 9: the lead leaves the victim only when it leads, for the first other member alive and not held.
static void exp_tentacle_move_lead_off(Character *victim)
{
    Player *p;
    Character *c;

    if (cur_character != victim && !(party_leader && party_leader->handle == victim->handle))
        return;
    for (p = party_list.next; p != &party_list; p = p->next) {
        if (p->handle == victim->handle)
            continue;
        c = living_from_handle(p->handle);
        if (c && !(c->flags & (CF_HIDDEN | CF_DEAD))) {
            party_set_leader(p);
            return;
        }
    }
}

// Trigger 7 of the grab move: take the victim.
static void exp_tentacle_take_victim(Character *tentacle, tentacle_info *t, Character *victim)
{
    int idx = exp_tentacle_index(t);
    move_info *flail;
    float d;

    if (victim == NULL) {
        exp_tentacle_trace("tentacle %d: grab impact, no target", idx);
        return;
    }
    if (victim->flags & (CF_HIDDEN | CF_DEAD)) {
        exp_tentacle_trace("tentacle %d: grab impact, victim 0x%x dead, dying or held (flags 0x%x)", idx,
                           victim->handle, victim->flags);
        return;
    }
    flail = charinfo_find_move(victim->info, 0xe, 0x8c, true);
    if (flail == NULL || flail->anim == -1) {
        exp_tentacle_trace("tentacle %d: grab impact, victim 0x%x has no flail pose", idx, victim->handle);
        return;
    }
    d = tentacle->distance_between_edges(victim, true, -1);
    if (!(d < t->reach + 1.0f)) {
        exp_tentacle_trace("tentacle %d: grab impact, victim 0x%x out of reach (%.2f, reach %.2f)", idx,
                           victim->handle, d, t->reach);
        return;
    }
    victim->instance->clear_state_anims();
    victim->instance->set_state_weight(flail->anim, 1.0f);
    t->victim = victim->handle;
    t->release_hp = tentacle->hp * 0.75f;   // slip 7: the tentacle's HP, not the victim's
    t->hold_timer.SetTimeout(30000);
    t->squeeze_timer.SetTimeout(0);
    victim->set_flag_2_4(-1);
    victim->flags |= CF_TENTACLE_HELD;
    exp_tentacle_set_anims(tentacle, 0x8d, 0x8d);
    exp_tentacle_move_lead_off(victim);
    exp_log("tentacle %d: grab, victim 0x%x, HP %.1f, tentacle HP %.1f, release below %.1f", idx, victim->handle,
            victim->hp, tentacle->hp, t->release_hp);
}

// tentacle_beast_ai_grab with the fixes.
static void exp_tentacle_ai_grab(ai_state *ai, Character *victim)
{
    Character *owner = ai->owner;
    tentacle_info *t = tentacle_beast_active_tentacle;

    ai_move_flags |= 4;
    if (owner->move_finished(ai->emerge_move)) {
        ai->emerge_move = -1;
        ai->go_idle();
        if (t->victim == -1 && t->thrown == -1) {
            exp_log("tentacle %d: grab move ended with nothing held, gate opens in 10 s", exp_tentacle_index(t));
            exp_tentacle_arm_grab_timer();   // slip 4
        }
        return;
    }
    if (owner->move_trigger_fired(7, 0) != 1)   // slip 8: -1 is "no such trigger"
        return;
    exp_tentacle_take_victim(owner, t, victim);
}

// ---- hold tick ----

// The squeeze tick of tentacle_beast_ai_frame: 3 damage with the grab on (the original heals 3).
static void exp_tentacle_hold_tick(Character *v, int source)
{
    float before;

    if (!exp_tentacle_on()) {
        v->add_hp(3.0f, source, 1, false);
        return;
    }
    before = v->hp;
    v->add_hp(-3.0f, source, 1, false);   // slip 6
    exp_tentacle_trace("tentacle %d: hold tick -3.0, victim 0x%x, HP %.1f -> %.1f",
                       exp_tentacle_index(tentacle_beast_active_tentacle), v->handle, before, v->hp);
}

// ---- release ----

// Trigger 8 of the throw move, with the victim test.
static void exp_tentacle_throw_victim(Character *tentacle, tentacle_info *t)
{
    int idx = exp_tentacle_index(t);
    Character *victim;
    matrix orient;
    vector3 pos;

    t->thrown = t->victim;
    t->victim = -1;
    t->throw_gravity = 0.0f;
    victim = living_from_handle(t->thrown);
    if (victim == NULL || exp_tentacle_is_down(victim)) {   // slip 8
        exp_log("tentacle %d: release, victim 0x%x %s, nothing thrown", idx, t->thrown, victim ? "dead" : "gone");
        if (victim)
            exp_tentacle_put_down(victim, victim->pos.y);
        t->thrown = -1;
        t->squeeze_timer.invalidate();
        exp_tentacle_set_anims(tentacle, 0x93, 0x95);
        exp_tentacle_arm_grab_timer();
        return;
    }
    // The original computes the grab bone here into locals it never reads; kept for the same call sequence.
    tentacle->instance->get_bone_world_transform(t->grab_tag, &tentacle->orient, &tentacle->pos, &orient, &pos);
    t->throw_velocity.x = (t->swipe_pos.x - victim->pos.x) * (2.0f / 3.0f);
    t->throw_velocity.y = (t->swipe_pos.y - victim->pos.y) * (2.0f / 3.0f);
    t->throw_velocity.z = (t->swipe_pos.z - victim->pos.z) * (2.0f / 3.0f);
    t->squeeze_timer.invalidate();
    exp_tentacle_set_anims(tentacle, 0x93, 0x95);
    if (victim == cur_character)
        game_camera.set_target(victim);
    if (exp_tentacle_index_ok(idx)) {
        exp_tentacle_throw_start_y[idx] = victim->pos.y;
        exp_tentacle_flight_time[idx] = 0.0f;
    }
    exp_log("tentacle %d: throw, victim 0x%x, HP %.1f, tentacle HP %.1f, velocity (%.2f %.2f %.2f)", idx,
            victim->handle, victim->hp, tentacle->hp, t->throw_velocity.x, t->throw_velocity.y, t->throw_velocity.z);
}

// tentacle_beast_ai_release with the fixes.
static void exp_tentacle_ai_release(ai_state *ai)
{
    Character *owner = ai->owner;

    ai_move_flags |= 4;
    if (owner->move_finished(ai->emerge_move)) {
        ai->go_idle();
        ai->emerge_move = -1;
        return;
    }
    if (owner->move_trigger_fired(8, 0) != 1)   // slip 8
        return;
    exp_tentacle_throw_victim(owner, tentacle_beast_active_tentacle);
}

// ---- battle end ----

// tentacle_begin_sink: a held or flying victim is put down now, then the tentacle sinks (slip 12).
static void exp_tentacle_begin_sink(tentacle_info *t, int ms)
{
    Player *p;
    Character *c;
    int goal;

    if (t->character == NULL)
        return;
    if (t->victim != -1 || t->thrown != -1)
        exp_tentacle_drop(t, "battle end / sink");
    t->sink_timer.SetTimeout(ms);
    t->flags |= TENTACLE_SINKING;
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c == NULL)
            continue;
        goal = c->ai.goal_find(4, AI_GOAL_ATTACK, 0, 0, t->character->handle, NULL, 0, 0x6c);
        if (goal != -1) {
            c->ai.goal_remove(goal);
            c->ai.go_idle();
        }
    }
}

// ---- frame hook ----

// One frame of the throw with the landing fixes (slip 13).
static void exp_tentacle_fly(tentacle_info *t, Character *r)
{
    int idx = exp_tentacle_index(t);
    float dt = reported_frametime;
    lobj_ray ray;
    lobj_hit hit;
    bool landed = false;
    float start_y;
    float aim_y;
    float floor_y;
    float damp;
    const char *why = "floor";

    memset((void *)&ray, 0, sizeof ray);
    memset((void *)&hit, 0, sizeof hit);
    ray.start = r->pos;
    ray.dir = t->throw_velocity * dt;
    ray._unk18 = 0.01f;
    ray.flags = 0;
    ray.require_flags = 0x40;
    ray.exclude_flags = 0;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits) {
        r->pos = ray.dir * hit.t + ray.start;
        r->pos.y += 0.5f;
        landed = true;
    } else {
        r->pos += ray.dir;
    }

    start_y = exp_tentacle_index_ok(idx) ? exp_tentacle_throw_start_y[idx] : r->pos.y;
    aim_y = t->swipe_pos.y;
    floor_y = start_y < aim_y ? start_y : aim_y;
    if (!landed && r->pos.y < floor_y - EXP_TENTACLE_FALL_LIMIT) {
        landed = true;
        why = "fell below the aim point";
    }
    if (exp_tentacle_index_ok(idx)) {
        exp_tentacle_flight_time[idx] += dt;
        if (!landed && exp_tentacle_flight_time[idx] > EXP_TENTACLE_FLIGHT_TIMEOUT) {
            landed = true;
            why = "flight timeout";
        }
    }

    damp = 1.0f - dt * 0.75f;
    t->throw_velocity.x *= damp;
    t->throw_velocity.z *= damp;
    t->throw_velocity.y = t->throw_velocity.y - dt * t->throw_gravity;
    t->throw_gravity = dt * 9.8f + t->throw_gravity;
    if (!landed)
        return;
    t->thrown = -1;
    exp_tentacle_arm_grab_timer();   // 0x4c7c28: the gate opens again 10 s after a landing
    exp_tentacle_put_down(r, start_y);
    exp_log("tentacle %d: landing (%s), victim 0x%x, HP %.1f, at (%.2f %.2f %.2f) after %.2f s", idx, why,
            r->handle, r->hp, r->pos.x, r->pos.y, r->pos.z,
            exp_tentacle_index_ok(idx) ? exp_tentacle_flight_time[idx] : 0.0f);
}

// tentacle_beast_frame_hook with the fixes (slip 11: a gone or dead victim is put down at once).
static void exp_tentacle_frame_hook(Character *c)
{
    tentacle_info *t = tentacle_find(c);
    Character *v;
    Character *r;
    vector3 p;

    if (t == NULL)
        return;
    if (t->victim != -1) {
        v = living_from_handle(t->victim);
        if (v == NULL || exp_tentacle_is_down(v)) {
            exp_tentacle_drop(t, v ? "held victim dead" : "held victim gone");
        } else {
            c->instance->get_bone_world_transform(t->grab_tag, &c->orient, &c->pos, &v->orient, &p);
            p.x -= v->orient.uvec.x * 1.5f;
            p.y -= v->orient.uvec.y * 1.5f;
            p.z -= v->orient.uvec.z * 1.5f;
            v->pos = p;
        }
    }
    if (t->thrown == -1)
        return;
    r = living_from_handle(t->thrown);
    if (r == NULL || exp_tentacle_is_down(r)) {
        exp_tentacle_drop(t, r ? "thrown victim dead" : "thrown victim gone");
        return;
    }
    exp_tentacle_fly(t, r);
}
#endif // EXPERIMENTAL_BUILD

// 0x4c6520  Confidence: exact (VC6 byte match, D21804, D26200; was likely). The candidate list holds 5: a
// held character first, then the visible party-type entities. The squeeze (move 2, chance 0.3) goes to the nearest candidate within 3 of the
// squeeze point; else the swipe (move 1) to the first candidate within reach. The grab (move 86) is chosen
// when frand() < 0, i.e. never (NOTES, game bugs).
int ai_state::tentacle_beast_ai_select_action()
{
    Character *list[5];
    Character *c;
    move_info *move;
    float best;
    float d;
    float r;
    int count;
    int i;
    int j;

#ifdef EXPERIMENTAL_BUILD
    if (exp_tentacle_on())
        return exp_tentacle_select_action(this);
#endif
    if (owner->combat.is_busy() == true)
        return 5;
    if (owner->move_playing(emerge_move, NULL))
        return 5;
    tentacle_beast_active_tentacle->move = NULL;
    if (!tentacle_beast_active_tentacle->think_timer.valid())
        tentacle_beast_active_tentacle->think_timer.SetRandomTimeout(2500, 5000);
    if (!tentacle_beast_active_tentacle->think_timer.elapsed())
        return 5;
    tentacle_beast_active_tentacle->think_timer.invalidate();
    count = 0;
    for (i = 0; i < 5; i++) {
        if (tentacle_beast_active_tentacle->slots[i].handle == -1)
            continue;
        c = living_from_handle(tentacle_beast_active_tentacle->slots[i].handle);
        if (!(c->flags & CF_DEAD)) {
            list[0] = c;
            count = 1;
            break;
        }
    }
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        // D21804: the two flags tested separately (8966 folds them into the binary's one `test byte,
        // 0x88` immediate; the combined mask is hoisted into BL and pushes 7 into EBP, spilling this).
        if (c->type != 7 || (c->flags & CF_HIDDEN) || (c->flags & CF_DEAD))
            continue;
        for (j = 0; j < count; j++) {
            if (list[j] == c)
                goto next;
        }
        list[count++] = c;
        if (count >= 5)
            break;
    next:;
    }
    if (count == 0)
        return 4;
    r = frand();
    if (r < 0.0f)
        move = tentacle_move_86;
    else if (r < 0.3f)
        move = tentacle_move_2;
    else
        move = tentacle_move_1;
    tentacle_beast_active_tentacle->move = NULL;
    if (move == tentacle_move_2) {
        best = 1000.0f;
        for (i = 0; i < count; i++) {
            vector3 a = list[i]->pos;
            vector3 b = tentacle_beast_active_tentacle->squeeze_pos;
            // D26200: b.y zeroed before a.y (the binary's store order; the other order swaps the two stores).
            b.y = 0.0f;
            a.y = 0.0f;
            d = distance(&a, &b);
            if (d < 3.0f && d < best) {
                best = d;
                tentacle_beast_active_tentacle->move = tentacle_move_2;
                tentacle_beast_active_tentacle->target = list[i]->handle;
            }
        }
        if (tentacle_beast_active_tentacle->move)
            return 3;
    }
    for (i = 0; i < count; i++) {
        if (owner->distance_between_edges(list[i], true, -1) < tentacle_beast_active_tentacle->reach) {
            tentacle_beast_active_tentacle->move = tentacle_move_1;
            tentacle_beast_active_tentacle->target = list[i]->handle;
            return 3;
        }
    }
    return 4;
}

// 0x4c6800  Confidence: exact. The grab move (0, 0x86) and sub-mode AIS_TENTACLE_GRAB; no new grab until
// the grab timer is rearmed.
void ai_state::tentacle_beast_start_grab()
{
    move_info *m = owner->find_move(0, 0x86);
    emerge_move = owner->play_move((int)m, 1.0f, false, false, false);
    set_mode(AI_STATE_COMBAT, AIS_TENTACLE_GRAB);
    tentacle_beast_grab_ready = 0;
    tentacle_beast_grab_timer.invalidate();
}

// 0x4c6850  Confidence: exact (VC6 byte match, match23; was likely). Sub-mode AIS_TENTACLE_GRAB: at the move's trigger 7 the victim plays its
// held pose (0xe, 0x8c), is held for 30 s or until it has lost a quarter of its HP, and loses its stance;
// the tentacle switches to its holding anims (0xe, 0x8d) and the party lead goes to the first other member.
void ai_state::tentacle_beast_ai_grab(Character *victim)
{
    move_info *m;
    int anim;
    Player *p;

#ifdef EXPERIMENTAL_BUILD
    if (exp_tentacle_on()) {
        exp_tentacle_ai_grab(this, victim);
        return;
    }
#endif
    ai_move_flags |= 4;
    if (owner->move_finished(emerge_move)) {
        emerge_move = -1;
        go_idle();
        return;
    }
    if (!owner->move_trigger_fired(7, 0) || victim == NULL)
        return;
    m = charinfo_find_move(victim->info, 0xe, 0x8c, true);
    if (m == NULL || (anim = m->anim) == -1)
        return;
    victim->instance->clear_state_anims();
    victim->instance->set_state_weight(anim, 1.0f);
    tentacle_beast_active_tentacle->victim = victim->handle;
    tentacle_beast_active_tentacle->release_hp = victim->hp * 0.75f;
    tentacle_beast_active_tentacle->hold_timer.SetTimeout(30000);
    tentacle_beast_active_tentacle->squeeze_timer.SetTimeout(0);
    victim->set_flag_2_4(-1);
    victim->flags |= CF_TENTACLE_HELD;
    owner->anim_ready = charinfo_find_move(owner->info, 0xe, 0x8d, true)->anim;
    owner->anim_ready_hurt = owner->anim_ready;
    owner->update_locomotion_anim();
    for (p = party_list.next; p != &party_list; p = p->next) {
        if (p->handle != victim->handle) {
            party_set_leader(p);
            return;
        }
    }
}

// 0x4c69e0  Confidence: exact. The release move (0, 0x87) and sub-mode AIS_TENTACLE_RELEASE; the host
// sends the object update and SWI_BSS.
void ai_state::tentacle_beast_start_release()
{
    if (is_mode(AI_STATE_COMBAT, AIS_TENTACLE_RELEASE))
        return;
    if (owner->get_current_move())
        return;
    move_info *m = owner->find_move(0, 0x87);
    emerge_move = owner->play_move((int)m, 1.0f, true, false, false);
    set_mode(AI_STATE_COMBAT, AIS_TENTACLE_RELEASE);
    priority = 99999;
    ai_move_flags |= 4;
    if (MP_IS_HOST()) {
        mp_send_obj_update(owner, 0);
        mp_send_swi_boss(owner, AI_STATE_COMBAT, AIS_TENTACLE_RELEASE);
    }
}

// 0x4c6a80  Confidence: exact (VC6 byte match, match9; was likely). Sub-mode AIS_TENTACLE_RELEASE: at trigger 8 the
// victim is thrown towards
// the swipe point (two thirds of the offset per second), the tentacle goes back to its normal anims (0xe,
// 0x93 / 0x95) and the camera follows a thrown cur_character. The grab bone's transform is computed into
// locals that are not used, and the victim is not NULL-checked (NOTES, game bugs).
void ai_state::tentacle_beast_ai_release()
{
    Character *victim;
    matrix orient;
    vector3 pos;

#ifdef EXPERIMENTAL_BUILD
    if (exp_tentacle_on()) {
        exp_tentacle_ai_release(this);
        return;
    }
#endif
    ai_move_flags |= 4;
    if (owner->move_finished(emerge_move)) {
        go_idle();
        emerge_move = -1;
        return;
    }
    if (!owner->move_trigger_fired(8, 0))
        return;
    tentacle_beast_active_tentacle->thrown = tentacle_beast_active_tentacle->victim;
    tentacle_beast_active_tentacle->victim = -1;
    tentacle_beast_active_tentacle->throw_gravity = 0.0f;
    victim = living_from_handle(tentacle_beast_active_tentacle->thrown);
    owner->instance->get_bone_world_transform(tentacle_beast_active_tentacle->grab_tag, &owner->orient,
                                              &owner->pos, &orient, &pos);
    vector3 d(tentacle_beast_active_tentacle->swipe_pos.x - victim->pos.x,
              tentacle_beast_active_tentacle->swipe_pos.y - victim->pos.y,
              tentacle_beast_active_tentacle->swipe_pos.z - victim->pos.z);
    tentacle_beast_active_tentacle->throw_velocity = d * (2.0f / 3.0f);
    tentacle_beast_active_tentacle->squeeze_timer.invalidate();
    owner->anim_ready = charinfo_find_move(owner->info, 0xe, 0x93, true)->anim;
    owner->anim_ready_hurt = charinfo_find_move(owner->info, 0xe, 0x95, true)->anim;
    owner->update_locomotion_anim();
    if (victim == cur_character)
        game_camera.set_target(victim);
}

// 0x4c6c20  Confidence: exact.
void tentacle_begin_rise(tentacle_info *t)
{
    t->sink_timer.SetTimeout(7000);
    t->flags |= TENTACLE_RISING;
}

// 0x4c6c50  Confidence: exact (VC6 byte match, match23; was likely). A tentacle that holds someone releases first; one whose thrown victim is
// still flying stays up. The party members stop attacking it.
void tentacle_begin_sink(tentacle_info *t, int ms)
{
    Player *p;
    Character *c;
    int goal;

#ifdef EXPERIMENTAL_BUILD
    if (exp_tentacle_on()) {
        exp_tentacle_begin_sink(t, ms);
        return;
    }
#endif
    if (t->victim != -1) {
        t->character->ai.tentacle_beast_start_release();
        return;
    }
    if (t->thrown != -1)
        return;
    t->sink_timer.SetTimeout(ms);
    t->flags |= TENTACLE_SINKING;
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c == NULL)
            continue;
        goal = c->ai.goal_find(4, AI_GOAL_ATTACK, 0, 0, t->character->handle, NULL, 0, 0x6c);
        if (goal != -1) {
            c->ai.goal_remove(goal);
            c->ai.go_idle();
        }
    }
}

// 0x4c6d10  Confidence: likely (the vector and matrix locals are the zero / identity frame of the squeeze
// tag). The first call of a level clears the records; each tentacle takes the first free record and
// measures its tags. The record index is not bounded (a fifth tentacle would write past the array).
void ai_state::tentacle_beast_boss_load()
{
    vector3 zero;
    matrix out;
    matrix identity;
    tentacle_info *t;
    int i;
    int tag;
    ca_character *ca;

    zero.zero();
    identity.make_identity();
    if (!tentacle_beast_loaded) {
        for (i = 0; i < NUM_TENTACLES; i++)
            tentacle_beast_tentacles[i].character = NULL;
        tentacle_beast_loaded = true;
        tentacle_beast_battle_started = false;
        tentacle_beast_tentacles_lost = 0;
        tentacle_beast_tentacles_up = 0;
        tentacle_beast_grab_ready = 1;
        tentacle_beast_grab_timer.SetTimeout(0);
    }
    for (i = 0; i < NUM_TENTACLES; i++) {
        if (tentacle_beast_tentacles[i].character == NULL)
            break;
    }
    tentacle_beast_active_tentacle = &tentacle_beast_tentacles[i];
    tentacle_beast_active_tentacle->character = owner;
    tentacle_beast_active_tentacle->hp_mark = owner->hp / (float)owner->get_max_stat(0) - 0.3f;
    tentacle_beast_active_tentacle->thrown = -1;
    tentacle_beast_active_tentacle->victim = -1;
    tentacle_beast_active_tentacle->sink_timer.invalidate();
    tentacle_beast_active_tentacle->hold_timer.invalidate();
    tentacle_beast_active_tentacle->squeeze_timer.invalidate();
    tentacle_beast_active_tentacle->think_timer.SetRandomTimeout(2500, 5000);
    tentacle_beast_active_tentacle->hold_time = 0.0f;
    tentacle_beast_active_tentacle->up_time = 0.0f;
    tentacle_beast_active_tentacle->splash_fx = -1;
    tentacle_beast_active_tentacle->death_anim = -1;
    tentacle_beast_active_tentacle->flags = 0;
    tentacle_beast_active_tentacle->target = -1;
    ca = owner->instance->character;
    tag = ca->find_mesh_tag("$tag-27-swipe");
    owner->instance->get_bone_world_transform(tag, &owner->orient, &owner->pos, &out,
                                              &tentacle_beast_active_tentacle->swipe_pos);
    t = tentacle_beast_active_tentacle;
    t->reach = (owner->pos - t->swipe_pos).len();
    tag = ca->find_mesh_tag("$tag-35-squeeze");
    owner->instance->get_bone_world_transform(tag, &identity, &zero, &out,
                                              &tentacle_beast_active_tentacle->squeeze_pos);
    tentacle_beast_active_tentacle->squeeze_pos += owner->pos;
    tag = ca->find_mesh_tag("$tag-26-pickup");
    owner->instance->get_bone_world_transform(tag, &owner->orient, &owner->pos, &out,
                                              &tentacle_beast_active_tentacle->pickup_pos);
    tentacle_beast_active_tentacle->grab_tag = ca->find_mesh_tag("$tag-grab-point");
    tentacle_move_1 = owner->find_move(0, 1);
    tentacle_move_2 = owner->find_move(0, 2);
    tentacle_move_86 = owner->find_move(0, 0x86);
    tentacle_move_87 = owner->find_move(0, 0x87);
    for (i = 0; i < 5; i++)
        tentacle_beast_active_tentacle->slots[i].handle = -1;
    owner->combat.set_owner(owner);
    go_idle();
    owner->cur_anim = -1;
    owner->flags |= 0x20000;
}

// 0x4c7050  Confidence: exact. boss_ai_unload case 0x93.
void tentacle_beast_unload()
{
    tentacle_beast_loaded = false;
}

// 0x4c7060  Confidence: exact (a tail call). boss_ai_idle_dispatch case 0x93: straight to the attack sub-mode.
void ai_state::tentacle_beast_ai_idle()
{
    set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT);
}

// 0x4c7070  Confidence: exact (VC6 byte match, D23400: the AIS_GET_CLOSE case written last; was likely)
// (the result switch and the sub-mode switch, byte table 0x4c7300 from 0xf).
// The stance toggle ends a rise (counted up) or a dive. While the battle runs and the tentacle is up: the
// time above water, the victim's squeeze tick (+3 HP every 6 s: a heal; NOTES, game bugs), and while it
// holds someone in the attack sub-mode only the hold time; otherwise a choice and the sub-mode action.
void ai_state::tentacle_beast_ai_frame()
{
    Character *target;
    Character *v;
    int result;

    tentacle_beast_active_tentacle = tentacle_find(owner);
    if (tentacle_beast_active_tentacle == NULL)
        return;
    if (is_combat_e_or_sneak_attack()) {
        if (combat_toggle_ready_stance() == true) {
            if (!(tentacle_beast_active_tentacle->flags & TENTACLE_SUBMERGED)) {
                tentacle_beast_active_tentacle->flags &= ~TENTACLE_RISING;
                tentacle_beast_tentacles_up++;
                set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT);
            } else {
                tentacle_beast_active_tentacle->flags &= ~TENTACLE_SINKING;
                go_idle();
            }
        }
        return;
    }
    if (!tentacle_beast_battle_started)
        return;
    if (tentacle_beast_active_tentacle->flags & (TENTACLE_SUBMERGED | TENTACLE_RISING | TENTACLE_SINKING))
        return;
    tentacle_beast_active_tentacle->up_time = reported_frametime + tentacle_beast_active_tentacle->up_time;
    if (tentacle_beast_active_tentacle->squeeze_timer.elapsed()) {
        v = living_from_handle(tentacle_beast_active_tentacle->victim);
        if (v)
#ifdef EXPERIMENTAL_BUILD
            exp_tentacle_hold_tick(v, owner->handle);   // -3 with the grab on, else the original +3
#else
            v->add_hp(3.0f, owner->handle, 1, false);
#endif
        tentacle_beast_active_tentacle->squeeze_timer.SetTimeout(6000);
    }
    if (tentacle_beast_active_tentacle->victim != -1 && is_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT)) {
        if (tentacle_beast_active_tentacle->hold_time < 7.5f)
            tentacle_beast_active_tentacle->hold_time = 11.25f;
        tentacle_beast_active_tentacle->hold_time = reported_frametime + tentacle_beast_active_tentacle->hold_time;
        return;
    }
    target = living_from_handle(tentacle_beast_active_tentacle->target);
    result = tentacle_beast_ai_select_action();
    switch (result) {
    case 0:
        go_idle();
        break;
    case 3:
        if (tentacle_beast_active_tentacle->target != this->target) {
            owner->combat.target = this->target = tentacle_beast_active_tentacle->target;
            target = living_from_handle(this->target);
        }
        tentacle_beast_active_tentacle->hold_time = 0.0f;
        break;
    case 4:
        tentacle_beast_active_tentacle->hold_time = reported_frametime + tentacle_beast_active_tentacle->hold_time;
        break;
    }
    if (result == 3 && tentacle_beast_active_tentacle->move == tentacle_move_86) {
        tentacle_beast_start_grab();
        return;
    }
    switch (submode) {
    case AIS_CLOSE_COMBAT:
        attack_range = owner->info->attack_radius;
        if (tentacle_beast_active_tentacle->move)
            owner->combat.update(tentacle_beast_active_tentacle->move->id);
        break;
    case AIS_TENTACLE_GRAB:
        tentacle_beast_ai_grab(target);
        break;
    case AIS_TENTACLE_RELEASE:
        tentacle_beast_ai_release();
        break;
    case AIS_GET_CLOSE:                 // D23400: last, so its set_mode falls into the exit block
        set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT);
        break;
    }
}

// 0x4c7330  Confidence: exact. The record of a tentacle character, NULL none.
tentacle_info *tentacle_find(Character *c)
{
    int i;

    for (i = 0; i < NUM_TENTACLES; i++) {
        if (tentacle_beast_tentacles[i].character == c)
            return &tentacle_beast_tentacles[i];
    }
    return NULL;
}

// 0x4c7360  Confidence: exact (VC6 byte match, match9; was likely). boss_ai_check_stage case 0x93: the end of a rise
// / dive flips the tentacle;
// a tentacle at HP 0 releases its victim, then plays its death move (6, 0x22) once (one more lost, one less
// up) and is submerged and deactivated when it ends; a living one releases below release_hp or after 30 s.
void ai_state::tentacle_beast_check_stage()
{
    tentacle_info *t = tentacle_find(owner);

    if (t == NULL)
        return;
    if (t->sink_timer.elapsed() == true) {
        t->sink_timer.invalidate();
        if (t->flags & TENTACLE_SUBMERGED)
            tentacle_beast_tentacle_resurface(t);
        else
            tentacle_beast_tentacle_submerge(t);
    }
    if (!tentacle_beast_battle_started)
        return;
    if (t->flags & (TENTACLE_SUBMERGED | TENTACLE_RISING | TENTACLE_SINKING))
        return;
    if (t->character->hp <= 0.0f) {
        if (t->victim != -1) {
            tentacle_beast_start_release();
            return;
        }
        if (t->character->get_current_move() == NULL && t->death_anim == -1) {
            tentacle_beast_tentacles_lost++;
            tentacle_beast_tentacles_up--;
            owner->flags |= CF_HIDDEN | CF_DEAD;
            move_info *m = owner->find_move(6, 0x22);
            t->death_anim = owner->play_move((int)m, 1.0f, true, true, false);
            return;
        }
        if (t->death_anim != -1 && !owner->move_playing(t->death_anim, NULL)) {
            t->flags |= TENTACLE_SUBMERGED;
            owner->obj_flags &= ~1;
        }
        return;
    }
    if (t->victim == -1 || is_mode(AI_STATE_COMBAT, AIS_TENTACLE_RELEASE))
        return;
    if (t->character->hp < t->release_hp || t->hold_timer.elapsed())
        tentacle_beast_start_release();
}

// 0x4c74f0  Confidence: exact. Out of combat, one less up, a splash.
void tentacle_beast_tentacle_submerge(tentacle_info *t)
{
    t->character->ai.goals_clear();
    t->character->ai.set_combat_mode_goal(false, 1000);
    t->flags |= TENTACLE_SUBMERGED;
    tentacle_beast_tentacles_up--;
    t->splash_fx = fx_attach_to_character("splash.vfx", t->character->handle, -1, -1, 0, 0);
}

// 0x4c7570  Confidence: exact (VC6 byte match, match23; was likely; the HP copy is a local). Back into combat with a splash; the HP mark is the
// current fraction - 0.3, not below 0.
void tentacle_beast_tentacle_resurface(tentacle_info *t)
{
    float hp = t->character->hp;
    float mark = hp / (float)t->character->get_max_stat(0) - 0.3f;

    if (0.0f > mark)
        mark = 0.0f;
    t->hp_mark = mark;
    t->character->ai.goals_clear();
    t->character->ai.set_combat_mode_goal(true, 1000);
    t->flags &= ~TENTACLE_SUBMERGED;
    t->up_time = 0.0f;
    t->hold_time = 0.0f;
    t->splash_fx = fx_attach_to_character("splash.vfx", t->character->handle, -1, -1, 0, 0);
}

// 0x4c7640  Confidence: exact (VC6 byte match, match23; was likely). The level script's frame call: the test mode only cycles the tentacles;
// in the battle the grab is allowed again when its timer runs out, and never while only one party member
// lives. The living_from_handle results are not NULL-checked.
void tentacle_beast_battle_update()
{
    Player *p;
    int alive;

    if (tentacle_test_mode == true) {
        tentacle_beast_cycle_tentacles();
        return;
    }
    if (!tentacle_beast_battle_started)
        return;
    if (!tentacle_beast_grab_ready && tentacle_beast_grab_timer.elapsed())
        tentacle_beast_grab_ready = 1;
    alive = 0;
    for (p = party_list.next; p != &party_list; p = p->next) {
        if (!(living_from_handle(p->handle)->flags & CF_DEAD))
            alive++;
    }
    if (alive == 1)
        tentacle_beast_grab_ready = 0;
}

// 0x4c76d0  Confidence: exact (VC6 byte match, match23; was likely; the counters are registers). tentacle_test: while none is rising or
// sinking, the last tentacle that is up dives (5 s); with all down, the one after the last cycled rises.
void tentacle_beast_cycle_tentacles()
{
    int sinking = 0;
    int rising = 0;
    int up = -1;
    int i;

    for (i = 0; i < NUM_TENTACLES; i++) {
        if (tentacle_beast_tentacles[i].flags & TENTACLE_RISING)
            rising++;
        else if (tentacle_beast_tentacles[i].flags & TENTACLE_SINKING)
            sinking++;
        else if (!(tentacle_beast_tentacles[i].flags & TENTACLE_SUBMERGED))
            up = i;
    }
    if (rising || sinking)
        return;
    if (up != -1) {
        tentacle_begin_sink(&tentacle_beast_tentacles[up], 5000);
        tentacle_last_cycled = up;
        return;
    }
    tentacle_begin_rise(&tentacle_beast_tentacles[(tentacle_last_cycled + 1) % 4]);
}

// 0x4c7770  Confidence: exact. boss attack kind: 0 the swipe, 1 the squeeze, 2 the grab, -1 none.
int tentacle_get_attack_kind(Character *c)
{
    tentacle_info *t = tentacle_find(c);

    if (t == NULL)
        return -1;
    if (t->move == tentacle_move_1)
        return 0;
    if (t->move == tentacle_move_86)
        return 2;
    return t->move == tentacle_move_2 ? 1 : -1;
}

// 0x4c77c0  Confidence: exact (VC6 byte match, match9; was likely). The swipe hits the members of cur_character's
// group within reach (by the
// edges) that are visible and fail their defence (the members are not NULL-checked).
bool tentacle_collect_area_hits(Character *c)
{
    tentacle_info *t = tentacle_find(c);
    ai_group *group;
    Character *m;
    int i;

    if (t == NULL)
        return false;
    if (t->move != tentacle_move_1)
        return false;
    t->num_hits = 0;
    group = cur_character->ai.group;
    if (group == NULL)
        return false;
    for (i = 0; i < group->count; i++) {
        m = living_from_handle(group->members[i]);
        if (c->distance_between_edges(m, true, -1) < t->reach && !(m->flags & CF_HIDDEN)
            && living_combat_test_defense(&c->combat, &m->combat))
#ifdef EXPERIMENTAL_BUILD
        {
            // G251 (with the grab on): four slots; a fifth hit wrote over death_anim.
            if (!exp_tentacle_on() || t->num_hits < 4)
                t->hits[t->num_hits++] = group->members[i];
        }
#else
            t->hits[t->num_hits++] = group->members[i];
#endif
    }
    return t->num_hits > 0 ? true : false;
}

// 0x4c78b0  Confidence: exact.
void tentacle_apply_area_hits(Character *c)
{
    tentacle_info *t = tentacle_find(c);
    Character *m;
    int i;

    if (t == NULL)
        return;
    for (i = 0; i < t->num_hits; i++) {
        m = living_from_handle(t->hits[i]);
        if (m)
            living_combat_resolve_hit_vs(&c->combat, &m->combat);
    }
}

// 0x4c7910  Confidence: exact. boss_move_in_range_dispatch case 0x93.
bool tentacle_target_in_reach(Character *boss, Character *target)
{
    tentacle_info *t = tentacle_find(boss);

    if (t && t->move == tentacle_move_1 && boss->distance_between_edges(target, true, -1) < t->reach)
        return true;
    return false;
}

// 0x4c7960  Confidence: likely (the landing's vector temporaries: the position is rebuilt through the
// out-of-line vector3 constructor and copied once more through the copy constructor; D16901: step as
// `throw_velocity * reported_frametime` (one frametime load), ray / hit declared after the thrown test (the
// ray constructor's store comes there), the landing and the miss through vector3 operators and fwd assigned
// with operator= (the binary's discarded out-of-line copy); D23401: `down = uvec * 1.5f`, `p -= down` and
// `up.set(0, 1, 0)` spend the inline budget as the binary (its out-of-line calls now the binary's), and fwd / up
// in a block share the frame slots (frame 0xa8 as the binary); left: the step product's schedule against the
// ray.start copy). boss_ai_do_frame_hook
// case 0x93: a held victim hangs 1.5 below the grab bone (its orientation is the bone's); a thrown one
// flies with the throw velocity (horizontal damping 0.75 per second, gravity growing by 9.8 per second)
// until the level ray from its position along this frame's step hits ground with flag 0x40. On landing it
// stands 0.5 above the hit, faces level, finds the ground and plays its land move (8, 1); the grab waits 10 s.
void tentacle_beast_frame_hook(Character *c)
{
    tentacle_info *t = tentacle_find(c);
    Character *v;
    Character *r;
    vector3 p;
    bool landed;
    float damp;

    if (t == NULL)
        return;
#ifdef EXPERIMENTAL_BUILD
    if (exp_tentacle_on()) {
        exp_tentacle_frame_hook(c);
        return;
    }
#endif
    v = living_from_handle(t->victim);
    if (v) {
        c->instance->get_bone_world_transform(t->grab_tag, &c->orient, &c->pos, &v->orient, &p);
        vector3 down = v->orient.uvec * 1.5f;     // D23401
        p -= down;
        v->pos = p;
    }
    r = living_from_handle(t->thrown);
    if (r == NULL)
        return;
    vector3 step = t->throw_velocity * reported_frametime;
    landed = false;
    lobj_ray ray;
    lobj_hit hit;
    ray.start = r->pos;
    ray.dir = step;
    ray._unk18 = 0.01f;
    ray.flags = 0;
    ray.require_flags = 0x40;
    ray.exclude_flags = 0;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits) {
        r->pos = ray.dir * hit.t + r->pos;
        r->pos.y += 0.5f;
        landed = true;
    } else {
        r->pos += ray.dir;
    }
    damp = 1.0f - reported_frametime * 0.75f;
    t->throw_velocity.x *= damp;
    t->throw_velocity.z *= damp;
    t->throw_velocity.y = t->throw_velocity.y - reported_frametime * t->throw_gravity;
    t->throw_gravity = reported_frametime * 9.8f + t->throw_gravity;
    if (!landed)
        return;
    r->flags &= ~CF_TENTACLE_HELD;
    r->ai.go_idle();
    {
        vector3 fwd;
        fwd = r->orient.fvec;
        vector3 up;
        fwd.y = 0.0f;
        up.set(0.0f, 1.0f, 0.0f);
        r->orient.set_from_forward_up(fwd, up);
    }
    r->find_ground();
    t->thrown = -1;
    tentacle_beast_grab_timer.SetTimeout(10000);
    move_info *m = r->find_move(8, 1);
    if (m)
        r->set_flag_2_4(r->play_move((int)m, 1.0f, false, false, false));
    else
        r->clear_flag_2_4();
}

// 0x4c7c90  Confidence: exact. static_init_ccmd_tentacles: "jmp 0x4c7ca0" (entry 331).
// 0x4c7ca0  Confidence: exact. init_ccmd_tentacles (console_command constructor).
console_command ccmd_tentacles("tentacles", "Start battle with tentacle beasts", cmd_tentacles);   // 0x248c788

// 0x4c7cc0  Confidence: exact (VC6 byte match, m-1020 w9; was likely) (the species switch is an if chain in the binary). Joseph and Flece are put
// at "$npc034" (Flece 2 back on every axis; Joseph leads), Jekhar and Rosalind are hidden; the camera snaps
// to cur_character; all four tentacles enter the stance, all but a random one dive (7 s); the battle starts.
void cmd_tentacles()
{
    navpoint *nav = navpoint_find("$npc034");
    Player *p;
    Character *c;
    int species;
    int keep;
    int i;

    for (p = party_list.next; p != &party_list; p = p->next) {
        species = char_infos[p->char_info].species;
        if (species == SPECIES_JOSEPH) {
            c = living_from_handle(p->handle);
            c->pos = nav->pos;
            c->layer = nav->layer;
            party_set_leader(p);
        } else if (species == SPECIES_FLECE) {
            c = living_from_handle(p->handle);
            c->pos = nav->pos;
            c->layer = nav->layer;
            c->pos -= vector3(2.0f, 2.0f, 2.0f);
        } else if (species == SPECIES_ROSALIND || species == SPECIES_JEKHAR) {
            c = living_from_handle(p->handle);
            c->flags |= CF_HIDDEN;
        }
    }
    game_camera.set_target(cur_character);
    game_camera.snap_to_target(1, 1);
    keep = (int)floor(frand() * 4.0f);
    for (i = 0; i < NUM_TENTACLES; i++) {
        c = tentacle_beast_tentacles[i].character;
        c->flags |= CF_COMBAT_READY;
        c->ai.go_idle();
        if (i != keep)
            tentacle_begin_sink(&tentacle_beast_tentacles[i], 7000);
    }
    tentacle_beast_tentacles_up = 4;
    tentacle_beast_battle_started = true;
}

// 0x4c7e60  Confidence: exact. static_init_ccmd_tentacle_test: "jmp 0x4c7e70" (entry 332).
// 0x4c7e70  Confidence: exact. init_ccmd_tentacle_test (console_command constructor).
console_command ccmd_tentacle_test("tentacle_test", "Makes tentacles go in and out of water quickly -- no attacks",
                                   cmd_tentacle_test);   // 0x248c778

// 0x4c7e90  Confidence: exact (VC6 byte match, m-1020 w9; was likely). As cmd_tentacles with the test mode on and the battle not started.
void cmd_tentacle_test()
{
    navpoint *nav;
    Player *p;
    Character *c;
    int species;
    int keep;
    int i;

    tentacle_test_mode = true;
    nav = navpoint_find("$npc034");
    for (p = party_list.next; p != &party_list; p = p->next) {
        species = char_infos[p->char_info].species;
        if (species == SPECIES_JOSEPH) {
            c = living_from_handle(p->handle);
            c->pos = nav->pos;
            c->layer = nav->layer;
            party_set_leader(p);
        } else if (species == SPECIES_FLECE) {
            c = living_from_handle(p->handle);
            c->pos = nav->pos;
            c->layer = nav->layer;
            c->pos -= vector3(2.0f, 2.0f, 2.0f);
        } else if (species == SPECIES_ROSALIND || species == SPECIES_JEKHAR) {
            c = living_from_handle(p->handle);
            c->flags |= CF_HIDDEN;
        }
    }
    game_camera.set_target(cur_character);
    game_camera.snap_to_target(1, 1);
    keep = (int)floor(frand() * 4.0f);
    for (i = 0; i < NUM_TENTACLES; i++) {
        c = tentacle_beast_tentacles[i].character;
        c->flags |= CF_COMBAT_READY;
        c->ai.go_idle();
        if (i != keep)
            tentacle_begin_sink(&tentacle_beast_tentacles[i], 7000);
    }
    tentacle_beast_tentacles_up = 4;
}
