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

// 0x4c6520  Confidence: likely (the candidate list holds 5: a held character first, then the visible
// party-type entities). The squeeze (move 2, chance 0.3) goes to the nearest candidate within 3 of the
// squeeze point; else the swipe (move 1) to the first candidate within reach. The grab (move 86) is chosen
// when frand() < 0, i.e. never (NOTES, game bugs).
int ai_state::tentacle_beast_ai_select_action()
{
    Character *list[5];
    Character *c;
    tentacle_info *t;
    move_info *move;
    float best;
    float d;
    float r;
    int count;
    int i;
    int j;

    if (owner->combat.is_busy() == true)
        return 5;
    if (owner->move_playing(emerge_move, NULL))
        return 5;
    t = tentacle_beast_active_tentacle;
    t->move = NULL;
    if (!t->think_timer.valid())
        t->think_timer.SetRandomTimeout(2500, 5000);
    if (!t->think_timer.elapsed())
        return 5;
    t->think_timer.invalidate();
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
        if (c->type != 7 || (c->flags & (CF_HIDDEN | CF_DEAD)))
            continue;
        for (j = 0; j < count; j++) {
            if (list[j] == c)
                break;
        }
        if (j < count)
            continue;
        list[count++] = c;
        if (count >= 5)
            break;
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
            vector3 a(list[i]->pos.x, 0.0f, list[i]->pos.z);
            vector3 b(tentacle_beast_active_tentacle->squeeze_pos.x, 0.0f, tentacle_beast_active_tentacle->squeeze_pos.z);
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
    emerge_move = owner->play_move((int)owner->find_move(0, 0x86), 1.0f, false, false, false);
    set_mode(AI_STATE_COMBAT, AIS_TENTACLE_GRAB);
    tentacle_beast_grab_ready = 0;
    tentacle_beast_grab_timer.invalidate();
}

// 0x4c6850  Confidence: likely. Sub-mode AIS_TENTACLE_GRAB: at the move's trigger 7 the victim plays its
// held pose (0xe, 0x8c), is held for 30 s or until it has lost a quarter of its HP, and loses its stance;
// the tentacle switches to its holding anims (0xe, 0x8d) and the party lead goes to the first other member.
void ai_state::tentacle_beast_ai_grab(Character *victim)
{
    move_info *m;
    int anim;
    Player *p;

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
    emerge_move = owner->play_move((int)owner->find_move(0, 0x87), 1.0f, true, false, false);
    set_mode(AI_STATE_COMBAT, AIS_TENTACLE_RELEASE);
    priority = 99999;
    ai_move_flags |= 4;
    if (MP_IS_HOST()) {
        mp_send_obj_update(owner, 0);
        mp_send_swi_boss(owner, AI_STATE_COMBAT, AIS_TENTACLE_RELEASE);
    }
}

// 0x4c6a80  Confidence: likely. Sub-mode AIS_TENTACLE_RELEASE: at trigger 8 the victim is thrown towards
// the swipe point (two thirds of the offset per second), the tentacle goes back to its normal anims (0xe,
// 0x93 / 0x95) and the camera follows a thrown cur_character. The grab bone's transform is computed into
// locals that are not used, and the victim is not NULL-checked (NOTES, game bugs).
void ai_state::tentacle_beast_ai_release()
{
    Character *victim;
    matrix orient;
    vector3 pos;

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
    tentacle_beast_active_tentacle->throw_velocity.x = d.x * (2.0f / 3.0f);
    tentacle_beast_active_tentacle->throw_velocity.y = d.y * (2.0f / 3.0f);
    tentacle_beast_active_tentacle->throw_velocity.z = d.z * (2.0f / 3.0f);
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

// 0x4c6c50  Confidence: likely. A tentacle that holds someone releases first; one whose thrown victim is
// still flying stays up. The party members stop attacking it.
void tentacle_begin_sink(tentacle_info *t, int ms)
{
    Player *p;
    Character *c;
    int goal;

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
    tag = owner->instance->character->find_mesh_tag("$tag-27-swipe");
    owner->instance->get_bone_world_transform(tag, &owner->orient, &owner->pos, &out,
                                              &tentacle_beast_active_tentacle->swipe_pos);
    t = tentacle_beast_active_tentacle;
    vector3 d(owner->pos.x - t->swipe_pos.x, owner->pos.y - t->swipe_pos.y, owner->pos.z - t->swipe_pos.z);
    t->reach = (float)sqrt(d.z * d.z + d.y * d.y + d.x * d.x);
    tag = owner->instance->character->find_mesh_tag("$tag-35-squeeze");
    owner->instance->get_bone_world_transform(tag, &identity, &zero, &out,
                                              &tentacle_beast_active_tentacle->squeeze_pos);
    tentacle_beast_active_tentacle->squeeze_pos.x += owner->pos.x;
    tentacle_beast_active_tentacle->squeeze_pos.y += owner->pos.y;
    tentacle_beast_active_tentacle->squeeze_pos.z += owner->pos.z;
    tag = owner->instance->character->find_mesh_tag("$tag-26-pickup");
    owner->instance->get_bone_world_transform(tag, &owner->orient, &owner->pos, &out,
                                              &tentacle_beast_active_tentacle->pickup_pos);
    tentacle_beast_active_tentacle->grab_tag = owner->instance->character->find_mesh_tag("$tag-grab-point");
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

// 0x4c7070  Confidence: likely (the result switch and the sub-mode switch, byte table 0x4c7300 from 0xf).
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
            v->add_hp(3.0f, owner->handle, 1, false);
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
            this->target = tentacle_beast_active_tentacle->target;
            owner->combat.target = tentacle_beast_active_tentacle->target;
            target = living_from_handle(this->target);
        }
        tentacle_beast_active_tentacle->hold_time = 0.0f;
        if (tentacle_beast_active_tentacle->move == tentacle_move_86) {
            tentacle_beast_start_grab();
            return;
        }
        break;
    case 4:
        tentacle_beast_active_tentacle->hold_time = reported_frametime + tentacle_beast_active_tentacle->hold_time;
        break;
    }
    switch (submode) {
    case AIS_CLOSE_COMBAT:
        attack_range = owner->info->attack_radius;
        if (tentacle_beast_active_tentacle->move)
            owner->combat.update(tentacle_beast_active_tentacle->move->id);
        break;
    case AIS_GET_CLOSE:
        set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT);
        break;
    case AIS_TENTACLE_GRAB:
        tentacle_beast_ai_grab(target);
        break;
    case AIS_TENTACLE_RELEASE:
        tentacle_beast_ai_release();
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

// 0x4c7360  Confidence: likely. boss_ai_check_stage case 0x93: the end of a rise / dive flips the tentacle;
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
            t->death_anim = owner->play_move((int)owner->find_move(6, 0x22), 1.0f, true, true, false);
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

// 0x4c7570  Confidence: likely (the HP copy is a local). Back into combat with a splash; the HP mark is the
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

// 0x4c7640  Confidence: likely. The level script's frame call: the test mode only cycles the tentacles;
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

// 0x4c76d0  Confidence: likely (the counters are registers). tentacle_test: while none is rising or
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

// 0x4c77c0  Confidence: likely. The swipe hits the members of cur_character's group within reach (by the
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
            t->hits[t->num_hits++] = group->members[i];
    }
    return t->num_hits > 0;
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
// out-of-line vector3 constructor and copied once more through the copy constructor). boss_ai_do_frame_hook
// case 0x93: a held victim hangs 1.5 below the grab bone (its orientation is the bone's); a thrown one
// flies with the throw velocity (horizontal damping 0.75 per second, gravity growing by 9.8 per second)
// until the level ray from its position along this frame's step hits ground with flag 0x40. On landing it
// stands 0.5 above the hit, faces level, finds the ground and plays its land move (8, 1); the grab waits 10 s.
void tentacle_beast_frame_hook(Character *c)
{
    tentacle_info *t = tentacle_find(c);
    Character *v;
    Character *r;
    lobj_ray ray;
    lobj_hit hit;
    vector3 p;
    vector3 step;
    bool landed;
    float damp;

    if (t == NULL)
        return;
    v = living_from_handle(t->victim);
    if (v) {
        c->instance->get_bone_world_transform(t->grab_tag, &c->orient, &c->pos, &v->orient, &p);
        vector3 down(v->orient.uvec.x * 1.5f, v->orient.uvec.y * 1.5f, v->orient.uvec.z * 1.5f);
        p.x -= down.x;
        p.y -= down.y;
        p.z -= down.z;
        v->pos = p;
    }
    r = living_from_handle(t->thrown);
    if (r == NULL)
        return;
    step.x = reported_frametime * t->throw_velocity.x;
    step.y = reported_frametime * t->throw_velocity.y;
    step.z = reported_frametime * t->throw_velocity.z;
    landed = false;
    ray.start = r->pos;
    ray.dir = step;
    ray._unk18 = 0.01f;
    ray.flags = 0;
    ray.require_flags = 0x40;
    ray.exclude_flags = 0;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits) {
        r->pos = vector3(ray.dir.x * hit.t + r->pos.x, ray.dir.y * hit.t + r->pos.y, ray.dir.z * hit.t + r->pos.z);
        r->pos.y += 0.5f;
        landed = true;
    } else {
        r->pos.x += ray.dir.x;
        r->pos.y += ray.dir.y;
        r->pos.z += ray.dir.z;
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
    vector3 fwd = r->orient.fvec;
    vector3 up(0.0f, 1.0f, 0.0f);
    fwd.y = 0.0f;
    r->orient.set_from_forward_up(fwd, up);
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

// 0x4c7cc0  Confidence: likely (the species switch is an if chain in the binary). Joseph and Flece are put
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
            c->pos.x -= 2.0f;
            c->pos.y -= 2.0f;
            c->pos.z -= 2.0f;
        } else if (species == SPECIES_ROSALIND || species == SPECIES_JEKHAR) {
            living_from_handle(p->handle)->flags |= CF_HIDDEN;
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

// 0x4c7e90  Confidence: likely. As cmd_tentacles with the test mode on and the battle not started.
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
            c->pos.x -= 2.0f;
            c->pos.y -= 2.0f;
            c->pos.z -= 2.0f;
        } else if (species == SPECIES_ROSALIND || species == SPECIES_JEKHAR) {
            living_from_handle(p->handle)->flags |= CF_HIDDEN;
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
