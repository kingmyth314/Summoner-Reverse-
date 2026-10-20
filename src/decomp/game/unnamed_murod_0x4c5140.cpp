// unnamed_murod_0x4c5140.cpp - Sum.exe v1.40, code 0x4c5140-0x4c5c1b: the Murod boss (species 0xb7): a
// caster that leaps next to a random party member and heals or cures itself. File name NOT known: a
// placeholder (src/decomp/README.md). No assert path, no strings, no strings.tbl block; cut by its static
// initialiser (entry 327, 0x588520) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_murod_timers (0x4c5140) to murod_filter_target (0x4c5b70-0x4c5c1b). The Murod Oni
// unit opens with its initialiser 0x4c5c20 (entry 328).
// Data: .data 0x59ddb0-0x59ddbf (murod_cure_statuses); .bss 0x248bf38-0x248bfef (murod_engaged first,
// murod_spell_timer last). No table file, no console command.
// Return codes of murod_ai_select_action: 0 no target, 1 a cast or a leap, 5 busy. Sub-modes 0x33 (the
// leap: vanish, move, reappear) and 0x34 (the arrival move).

#include <math.h>
#include <stdlib.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"

#define AIS_CLOSE_COMBAT  0x0f        // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE     0x16
#define AIS_GET_FAR       0x17
#define AIS_MUROD_LEAP    0x33        // names ours
#define AIS_MUROD_ARRIVE  0x34

#define SPELL_MUROD_HEAL  0           // the spells the boss casts at itself
#define SPELL_MUROD_CURE  1

// ---- .data --------------------------------------------------------------------------------------------
int murod_cure_statuses[4] = { STATUS_BLIND, STATUS_SILENCE, STATUS_CURSE, STATUS_POISON };   // 0x59ddb0

// ---- .bss (address order) -----------------------------------------------------------------------------
bool murod_engaged;                   // 0x248bf38
boss_threat murod_threat_table[BOSS_MAX_THREATS];      // 0x248bf3c
int  murod_target_handle;             // 0x248bfb4
int  murod_casting_spell;             // 0x248bfb8
int  murod_action22_count;            // 0x248bfbc
float murod_last_health;              // 0x248bfc0
int  murod_move_anim;                 // 0x248bfc4
vector3 murod_leap_dest;              // 0x248bfc8
vector3 murod_leap_dir;               // 0x248bfd4
GameTimer murod_attack_timer;         // 0x248bfe0
GameTimer murod_leap_timer;           // 0x248bfe4
GameTimer murod_heal_timer;           // 0x248bfe8
GameTimer murod_spell_timer;          // 0x248bfec

// 0x4c5140  Confidence: exact. static_init_murod_timers: "jmp 0x4c5150" (initialiser entry 327).
// 0x4c5150  Confidence: likely. init_murod_timers: the GameTimer constructor for the five threat records,
// then the attack, leap, heal and spell timers.

// 0x4c51a0  Confidence: likely (match31: the direction built with the vector3 constructor, z's frand first;
// the frame is 8-aligned for floor's double; the party list local holds 5).
// Five tries: a random living party member and a point 15 away from it in a random horizontal direction
// that is navigable for its size; then the vanish move (8, 0x8a), sub-mode AIS_MUROD_LEAP, the member
// becomes the target and the next leap waits 20 s. Without a point: 5 s.
bool ai_state::murod_start_leap()
{
    Character *members[5];
    Character *t;
    vector3 dir;
    float fcount;
    int count = 0;
    int tries;
    Player *p;
    Character *c;

    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c)
            members[count++] = c;
    }
    if (count == 0)
        goto fail;
    fcount = (float)count;
    tries = 4;
    do {
        t = members[(int)floor(frand() * fcount)];
        dir = vector3(frand() - 0.5f, 0.0f, frand() - 0.5f);
        if (dir.normalize_length() != 0.0f) {
            dir *= 15.0f;
            dir += t->pos;
            if (pf_point_is_navigable(&dir, t->info->size, t->layer))
                goto found;
        }
    } while (tries-- != 0);
fail:
    murod_leap_timer.SetTimeout(5000);
    return false;
found:
    murod_leap_dest = dir;
    murod_leap_dir.x = t->pos.x - dir.x;
    murod_leap_dir.y = t->pos.y - dir.y;
    murod_leap_dir.z = t->pos.z - dir.z;
    murod_move_anim = owner->play_move((int)owner->find_move(8, 0x8a), 1.0f, false, false, false);
    set_mode(AI_STATE_COMBAT, AIS_MUROD_LEAP);
    murod_target_handle = t->handle;
    murod_leap_timer.SetTimeout(20000);
    owner->set_6ac(0.0f);
    return true;
}

// 0x4c53c0  Confidence: exact (VC6 byte match, m-1019 w1, D10000). Sub-mode AIS_MUROD_LEAP: when the vanish move ends, the boss is put at the
// destination (a fresh floor search), faces the member, is shown again and plays the arrival move (8,
// 0x8b). Until then, from frame 73 of the move, the level renderer tints the boss (0 -> 1 over 12 frames)
// and its alpha falls from 255 after frame 85 (25.5 per frame).
void ai_state::murod_ai_leap()
{
    float t;
    float k;
    int alpha;

    if (owner->move_finished(murod_move_anim)) {
        owner->pos = murod_leap_dest;
        owner->flags &= ~CF_ON_GROUND;
        owner->find_ground();
        owner->orient.set_from_forward(murod_leap_dir);
        owner->obj_flags |= 1;
        owner->instance->_unkb4 = 0xff;
        move_info *m = owner->find_move(8, 0x8b);
        murod_move_anim = owner->play_move((int)m, 1.0f, false, false, false);
        set_mode(AI_STATE_COMBAT, AIS_MUROD_ARRIVE);
        priority = 99999;
        return;
    }
    t = owner->instance->get_anim_elapsed(owner->cur_move->anim);
    if (t < 73.0f)
        return;
    levelrender_fx_char = owner;
    k = (t - 73.0f) * (1.0f / 12.0f);
    if (k < 0.0f)
        k = 0.0f;
    else if (k > 1.0f)
        k = 1.0f;
    levelrender_fx_char_tint_b = k;
    levelrender_fx_char_tint_g = k;
    levelrender_fx_char_tint_r = k;
    alpha = 255 - (int)((t - 85.0f) * 25.5f);
    if (alpha < 0)
        alpha = 0;
    else if (alpha > 255)
        alpha = 255;
    owner->instance->_unkb4 = (ubyte)alpha;
}

// 0x4c5550  Confidence: exact. Sub-mode AIS_MUROD_ARRIVE.
void ai_state::murod_end_move()
{
    if (owner->move_finished(murod_move_anim))
        go_idle();
}

// 0x4c5570  Confidence: exact. Every 20 s: cure (spell 1) for blind, silence, curse or poison, else heal
// (spell 0) after losing a fifth of the HP since the last heal.
bool ai_state::murod_try_self_heal()
{
    int i;

    murod_heal_timer.SetTimeout(20000);
    for (i = 0; i < 4; i++) {
        if (owner->has_status(murod_cure_statuses[i])) {
            murod_cast_spell(SPELL_MUROD_CURE, owner);
            return true;
        }
    }
    if (owner->hp / murod_last_health < 0.8f) {
        murod_last_health = owner->hp;
        murod_cast_spell(SPELL_MUROD_HEAL, owner);
        return true;
    }
    return false;
}

// 0x4c5600  Confidence: exact (as evil_urath_cast_spell).
void ai_state::murod_cast_spell(int spell, Character *target)
{
    move_info *move = owner->find_move(7, spells[spell].variation);

    if (move == NULL)
        return;
    owner->cur_anim = owner->play_move((int)move, 1.0f, true, true, false);
    if (owner->cur_anim == -1)
        return;
    add_cast_at_target(spell, target, 5000);
    murod_move_anim = owner->cur_anim;
    murod_casting_spell = spell;
}

// 0x4c5680  Confidence: exact. boss_ai_idle_dispatch case 0xb7: murod_engaged is set at load, so the boss
// fights at once.
void ai_state::murod_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (murod_engaged == true)
        enter_combat();
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_unload case 0xb7 (boss_ai.h: folded).
void murod_unload()
{
}

// 0x4c56c0  Confidence: exact.
void ai_state::murod_boss_load()
{
    int i;

    murod_engaged = true;
    owner->flags |= CF_COMBAT_READY;
    murod_target_handle = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        murod_threat_table[i].handle = -1;
    murod_last_health = owner->hp;
    murod_heal_timer.SetTimeout(20000);
    murod_leap_timer.SetTimeout(20000);
    murod_spell_timer.SetTimeout(15000);
    murod_casting_spell = -1;
    murod_action22_count = 0;
}

// 0x4c5750  Confidence: exact. boss_ai_check_stage case 0xb7: the end of a cast; after the heal the HP
// mark moves up.
void ai_state::murod_check_stage()
{
    if (murod_casting_spell == -1 || !owner->move_finished(murod_move_anim))
        return;
    if (murod_casting_spell == SPELL_MUROD_HEAL && owner->hp > murod_last_health)
        murod_last_health = owner->hp;
    murod_casting_spell = -1;
}

// 0x4c57b0  Confidence: likely. More than one attacker cuts the leap wait to 5 s. Order: leap, self heal,
// the 15 s spell (8 at a flag-1 target without flag 8, else 7 unless flag 0x10), then 0x1a (no fireball
// out, 50 % / 60 %), 0x22 (60 %, at most twice in a row, not at a paralysed target) or 0xf.
int ai_state::murod_ai_select_action()
{
    boss_target list[BOSS_MAX_THREATS];
    Character *target;
    bool timer_up;
    int count;
    int index;
    int spell;
    int fireballs;
    unsigned int flags;
    float r;

    if (murod_casting_spell != -1 || submode == AIS_MUROD_LEAP || submode == AIS_MUROD_ARRIVE)
        return 5;
    target = living_from_handle(murod_target_handle);
    if (target && (target->flags & CF_HIDDEN))
        target = NULL;
    timer_up = murod_attack_timer.elapsed();
    count = boss_target_list_build(owner, list, murod_threat_table);
    if (count_attackers() > 1 && murod_leap_timer.GetRemainingTime() > 5000)
        murod_leap_timer.SetTimeout(5000);
    if (timer_up || !target) {
        murod_target_handle = boss_target_list_pick(list, count, murod_threat_table, murod_target_handle, false, 0);
        if (murod_target_handle == -1)
            return 0;
        target = living_from_handle(murod_target_handle);
        murod_set_attack_timer(5000, 15000);
    }
    if (murod_leap_timer.elapsed() && murod_start_leap())
        return 1;
    if (murod_heal_timer.elapsed() && murod_try_self_heal())
        return 1;
    if (murod_spell_timer.elapsed()) {
        murod_spell_timer.SetTimeout(15000);
        index = boss_target_list_find(list, count, murod_target_handle);
        if (index != -1) {
            flags = list[index].flags;
            if ((flags & 1) && !(flags & 8)) {
                murod_cast_spell(8, target);
                return 1;
            }
            if (!(flags & 0x10)) {
                murod_cast_spell(7, target);
                return 1;
            }
        }
    }
    fireballs = fx_count_spells_1a_1d_4e();
    r = frand();
    if (target->has_status(STATUS_PARALYZE)) {
        if (fireballs == 0 && r < 0.5f)
            spell = 0x1a;
        else
            spell = 0xf;
    } else if (r < 0.6f && murod_action22_count < 2) {
        spell = 0x22;
    } else if (fireballs == 0 && r < 0.6f) {
        spell = 0x1a;
    } else {
        spell = 0xf;
    }
    murod_cast_spell(spell, target);
    if (spell == 0x22)
        murod_action22_count++;
    else
        murod_action22_count = 0;
    return 1;
}

// 0x4c5a30  Confidence: exact.
void murod_set_attack_timer(int min_ms, int max_ms)
{
    murod_attack_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4c5a60  Confidence: exact (the sub-mode switch: case -1 has its own slot, byte table 0x4c5b2c; both
// go_idle paths share the one call placed last, D7700).
void ai_state::murod_ai_frame()
{
    int result;
    Character *target;

    result = murod_ai_select_action();
    if (result != 0) {
        target = living_from_handle(murod_target_handle);
        owner->combat.target = murod_target_handle;
        if (target && (target->flags & CF_HIDDEN))
            target = NULL;
        if (submode == AIS_MUROD_LEAP || submode == AIS_MUROD_ARRIVE || target != NULL) {
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
            case AIS_MUROD_LEAP:
                murod_ai_leap();
                return;
            case AIS_MUROD_ARRIVE:
                murod_end_move();
                return;
            }
            return;
        }
    }
    go_idle();
}

// 0x4c5b70  Confidence: exact. boss_target_filter_dispatch case 0xb7. Flags: 2 paralysed, 4 status 14
// (fire wall), 0x10 blind, 8 silenced, 1 the threat ratio [2] / [3] above 0.25. As in Evil Urath's filter
// the handle is read before the NULL test.
void murod_filter_target(Character *boss, boss_target *t)
{
    boss_threat *threat = NULL;
    int i;

    for (i = 0; i < BOSS_MAX_THREATS; i++) {
        if (murod_threat_table[i].handle == t->character->handle) {
            threat = &murod_threat_table[i];
            break;
        }
    }
    if (t->character == NULL)
        return;
    if (t->character->has_status(STATUS_PARALYZE))
        t->flags |= 2;
    if (t->character->has_status(STATUS_FIREWALL))
        t->flags |= 4;
    if (t->character->has_status(STATUS_BLIND))
        t->flags |= 0x10;
    if (t->character->has_status(STATUS_SILENCE))
        t->flags |= 8;
    if (threat && threat->threat[2] / threat->threat[3] > 0.25f)
        t->flags |= 1;
}
