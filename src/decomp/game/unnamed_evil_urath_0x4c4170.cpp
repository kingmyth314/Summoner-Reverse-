// unnamed_evil_urath_0x4c4170.cpp - Sum.exe v1.40, code 0x4c4170-0x4c513b: the Evil Urath boss (species 0xb1):
// spells only, and leaps between the temple floor and the two balconies by HP band. File name NOT known: a
// placeholder (src/decomp/README.md). No assert path, no strings.tbl block; cut by its static initialiser
// run (entries 325-326, 0x588518-0x58851c) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_evil_urath_timers (0x4c4170) to evil_urath_set_attack_timer (0x4c5110-0x4c513b).
// The Murod unit opens with its initialiser 0x4c5140 (entry 327).
// Data: .data 0x59dd3c-0x59ddaf (command name, help, combat message, "$temple_0%d", "$balc02_0%d",
// "$balc01_0%d"); .bss 0x248bd00-0x248bf33 (evil_urath_248bd00 first, ccmd_urath_combat last; Murod's
// data starts at 0x248bf38). The boss has no table file: the 0x140 bytes after the threat table (an attack
// table's size) and 0x248bf00 / 0x248bf04 / 0x248bf24 are never accessed, and evil_urath_current_move is
// only ever cleared.
// Leap flags (evil_urath_leap_flags, names ours): 1 on the floor, 2 on a balcony, 8 rising to a balcony,
// 4 coming down. Sub-modes: 0x23 leap start, 0x24 rise and turn, 0x25 travel, 0x26 land.
// Return codes of evil_urath_ai_select_action: 0 no target, 1 a spell or a leap (pending sub-mode 0x23),
// 4 nothing this think, 5 busy.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "summoner/summoner.h"

#define AIS_URATH_LEAP_START 0x23     // names ours
#define AIS_URATH_LEAP_RISE  0x24
#define AIS_URATH_LEAP_TRAVEL 0x25
#define AIS_URATH_LEAP_LAND  0x26

#define URATH_ON_FLOOR    0x1
#define URATH_ON_BALCONY  0x2
#define URATH_COMING_DOWN 0x4
#define URATH_GOING_UP    0x8

#define CF_URATH_LEAPING  0x1008      // Character +0x320 bits set for the leap (CF_HIDDEN | 0x1000)

// ---- .bss (address order) -----------------------------------------------------------------------------
int  evil_urath_248bd00;              // 0x248bd00 1 at load, never read
int  evil_urath_248bd04;              // 0x248bd04 0 at load, never read
unsigned int evil_urath_leap_flags;   // 0x248bd08
bool evil_urath_combat_enabled;       // 0x248bd0c
boss_threat evil_urath_threat_table[BOSS_MAX_THREATS];   // 0x248bd10
boss_attack evil_urath_unused_moves[BOSS_MAX_OFFENSE_POINTS];   // 0x248bd88 never accessed
boss_attack *evil_urath_current_move; // 0x248bec8 only cleared
int  evil_urath_pending_mode;         // 0x248becc
int  evil_urath_target_handle;        // 0x248bed0
GameTimer evil_urath_attack_timer;    // 0x248bed4
navpoint *evil_urath_leap_target;     // 0x248bed8
navpoint *evil_urath_teleports_a[3];  // 0x248bedc "$balc01_01".."03"
navpoint *evil_urath_teleports_b[3];  // 0x248bee8 "$temple_01".."03"
navpoint *evil_urath_teleports_c[3];  // 0x248bef4 "$balc02_01".."03"
int  evil_urath_248bf00[2];           // 0x248bf00 never accessed
float evil_urath_leap_rise_speed;     // 0x248bf08 the horizontal speed
float evil_urath_leap_fall_speed;     // 0x248bf0c
float evil_urath_leap_height;         // 0x248bf10
float evil_urath_leap_start_y;        // 0x248bf14
float evil_urath_leap_peak_y;         // 0x248bf18 the horizontal distance at which the descent starts
int  evil_urath_casting_spell;        // 0x248bf1c
int  evil_urath_cast_anim;            // 0x248bf20
int  evil_urath_248bf24;              // 0x248bf24 never accessed
// ccmd_urath_combat 0x248bf28 is defined with its initialiser below.

// 0x4c4170  Confidence: exact. static_init_evil_urath_timers: "jmp 0x4c4180" (initialiser entry 325).
// 0x4c4180  Confidence: likely. init_evil_urath_timers: the GameTimer constructor for the five threat records
// and evil_urath_attack_timer.

// 0x4c41b0  Confidence: exact. static_init_ccmd_urath_combat: "jmp 0x4c41c0" (entry 326).
// 0x4c41c0  Confidence: exact. init_ccmd_urath_combat (console_command constructor).
console_command ccmd_urath_combat("urath_combat", "Sets urath in combat state", cmd_urath_combat);   // 0x248bf28

// 0x4c41e0  Confidence: exact.
void cmd_urath_combat()
{
    evil_urath_combat_enabled = true;
    print_to_console("Setting urath' ai state to combat\n", 0);
}

// 0x4c4200  Confidence: likely (the parameter slots are reused as the two intersection parameters). True
// when a nav wall of the boss's size class crosses the line from the boss to the leap target (x/z). *dist
// gets the distance to the crossing; it is written for every crossing, so it holds the last one found,
// not the nearest (the nearest is kept in a local that is not used; NOTES, game bugs).
bool evil_urath_leap_check_path(Character *boss, int layer, float *dist)
{
    float nearest = 3.4028235e+38f;
    vector2 end;
    vector2 start;
    vector2 d;
    pf_grid_cell *cell;
    pf_edge *wall;
    float t;
    float u;
    float len;
    int cells;
    int k;
    int i;

    *dist = 3.4028235e+38f;
    start.x = boss->pos.x;
    start.y = boss->pos.z;
    end.x = evil_urath_leap_target->pos.x;
    end.y = evil_urath_leap_target->pos.z;
    cells = pf_cells_along_segment(layer, boss->info->size, &start, &end);
    for (k = 0; k < cells; k++) {
        cell = pf_grid_cell_results[k];
        for (i = 0; i < cell->num_walls; i++) {
            wall = cell->walls[i];
            if (!seg2d_intersect_params(&start, &end, wall->a, wall->b, &t, &u))
                continue;
            if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f)
                continue;
            d.x = end.x - start.x;
            d.y = end.y - start.y;
            len = d.len() * t;
            *dist = len;
            if (len < nearest)
                nearest = len;
        }
    }
    return *dist < 3.4028235e+38f;
}

// 0x4c4380  Confidence: exact. Going up: balcony 1 or 2 (frand above 0.5: balcony 1); coming down: the temple.
void evil_urath_pick_teleport_point()
{
    if (evil_urath_leap_flags & URATH_GOING_UP) {
        if (frand() > 0.5f)
            evil_urath_leap_target = evil_urath_teleports_a[rand() % 3];
        else
            evil_urath_leap_target = evil_urath_teleports_c[rand() % 3];
    }
    if (evil_urath_leap_flags & URATH_COMING_DOWN)
        evil_urath_leap_target = evil_urath_teleports_b[rand() % 3];
}

// 0x4c43f0  Confidence: exact (as machival_cast_spell with category 7 only).
void ai_state::evil_urath_cast_spell(int spell, Character *target)
{
    Character *c = owner;
    int variation = spells[spell].variation;
    move_info *move = c->find_move(7, variation);

    if (move == NULL)
        return;
    owner->cur_anim = owner->play_move((int)move, 1.0f, true, true, false);
    if (owner->cur_anim == -1)
        return;
    add_cast_at_target(spell, target, 5000);
    evil_urath_cast_anim = owner->cur_anim;
    evil_urath_casting_spell = spell;
}

// 0x4c4470  Confidence: exact. boss_target_filter_dispatch case 0xb1. Flags: 0x20 paralysed, 0x10
// poisoned, 2 blind, 4 silenced, 8 cursed, 0x40 farther than 3.5, 1 its threat record's [2] / [3] above
// 0.25. The threat search reads the candidate's handle before the NULL test (NOTES, game bugs).
void evil_urath_filter_target(Character *boss, boss_target *t)
{
    boss_threat *threat = NULL;
    int i;

    for (i = 0; i < BOSS_MAX_THREATS; i++) {
        if (evil_urath_threat_table[i].handle == t->character->handle) {
            threat = &evil_urath_threat_table[i];
            break;
        }
    }
    if (t->character == NULL)
        return;
    if (t->character->has_status(STATUS_PARALYZE))
        t->flags |= 0x20;
    if (t->character->has_status(STATUS_POISON))
        t->flags |= 0x10;
    if (t->character->has_status(STATUS_BLIND))
        t->flags |= 2;
    if (t->character->has_status(STATUS_SILENCE))
        t->flags |= 4;
    if (t->character->has_status(STATUS_CURSE))
        t->flags |= 8;
    vector3 d = t->character->pos - boss->pos;
    if ((float)sqrt(d.x * d.x + d.y * d.y + d.z * d.z) > 3.5f)
        t->flags |= 0x40;
    if (threat && threat->threat[2] / threat->threat[3] > 0.25f)
        t->flags |= 1;
}

// 0x4c4580  Confidence: exact. The unit's own copy of boss_target_list_find (0x4c3c70): a file-static or
// header function in the original, not merged by the linker.
int boss_target_list_find_b(boss_target *list, int count, int handle)
{
    int i;

    for (i = 0; i < count; i++) {
        if (list[i].character->handle == handle)
            return i;
    }
    return -1;
}

// folded (0x40acc0, empty_frame_hook_40acc0)  Confidence: likely. The empty call of evil_urath_boss_load
// (no arguments pushed); name ours.
void evil_urath_load_hook()
{
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_ai_unload case 0xb1 (boss_ai.h: folded).
void evil_urath_unload()
{
}

// 0x4c45b0  Confidence: exact. No table file and no offense points: the boss only casts. AP 10000.
void ai_state::evil_urath_boss_load()
{
    int i;

    evil_urath_248bd00 = 1;
    evil_urath_248bd04 = 0;
    owner->flags |= CF_COMBAT_READY;
    owner->ap = 10000;
    evil_urath_target_handle = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        evil_urath_threat_table[i].handle = -1;
    evil_urath_find_teleports();
    evil_urath_leap_target = NULL;
    evil_urath_leap_flags = URATH_ON_FLOOR;
    evil_urath_load_hook();
    evil_urath_leap_height = 0.0f;
    evil_urath_cast_anim = -1;
    evil_urath_casting_spell = -1;
}

// 0x4c4640  Confidence: exact.
void evil_urath_find_teleports()
{
    char name[64];
    int i;

    for (i = 0; i < 3; i++) {
        sprintf(name, "$balc01_0%d", i + 1);
        evil_urath_teleports_a[i] = navpoint_find(name);
    }
    for (i = 0; i < 3; i++) {
        sprintf(name, "$temple_0%d", i + 1);
        evil_urath_teleports_b[i] = navpoint_find(name);
    }
    for (i = 0; i < 3; i++) {
        sprintf(name, "$balc02_0%d", i + 1);
        evil_urath_teleports_c[i] = navpoint_find(name);
    }
}

// 0x4c46f0  Confidence: exact. The second call of boss_ai_check_stage case 0xb1: during the leap the
// height is the script's, not the floor's.
void ai_state::evil_urath_hold_height()
{
    if (submode == AIS_URATH_LEAP_RISE || submode == AIS_URATH_LEAP_TRAVEL || submode == AIS_URATH_LEAP_LAND)
        owner->pos.y = evil_urath_leap_height;
}

// 0x4c4710  Confidence: exact. boss_ai_idle_dispatch case 0xb1 (the combat flag: console, TempleInt2
// script). Clears Character flag 0x1000 when the fight starts.
void ai_state::evil_urath_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (evil_urath_combat_enabled == true) {
        evil_urath_current_move = NULL;
        owner->flags &= ~0x1000;
        enter_combat();
    }
}

// 0x4c4760  Confidence: exact (the sub-mode switch is a plain table of 4, 0x4c47d0).
void ai_state::evil_urath_ai_frame()
{
    int result = evil_urath_ai_select_action();

    if (result == 1) {
        if (evil_urath_pending_mode == AIS_URATH_LEAP_START) {
            set_mode(AI_STATE_COMBAT, AIS_URATH_LEAP_START);
            evil_urath_pending_mode = -1;
        }
    } else if (result == 0) {
        go_idle();
        return;
    }
    switch (submode) {
    case AIS_URATH_LEAP_START:
        evil_urath_ai_leap_start();
        break;
    case AIS_URATH_LEAP_RISE:
        evil_urath_ai_leap_rise();
        break;
    case AIS_URATH_LEAP_TRAVEL:
        evil_urath_ai_leap_travel();
        break;
    case AIS_URATH_LEAP_LAND:
        evil_urath_ai_leap_land();
        break;
    }
}

// 0x4c47e0  Confidence: exact. boss_ai_check_stage case 0xb1: no HP stages, only the end of a cast.
void ai_state::evil_urath_check_stage()
{
    if (evil_urath_casting_spell != -1 && owner->move_finished(evil_urath_cast_anim))
        evil_urath_casting_spell = -1;
}

// 0x4c4810  Confidence: exact. From the floor the boss goes up, from a balcony down (remembering the
// floor height first).
void ai_state::evil_urath_ai_leap_start()
{
    unsigned int f;

    owner->flags |= CF_URATH_LEAPING;
    f = evil_urath_leap_flags;
    if (f & URATH_ON_FLOOR) {
        f = (f & ~URATH_ON_FLOOR) | URATH_GOING_UP;
        evil_urath_leap_flags = f;
    }
    if (f & URATH_ON_BALCONY) {
        f = (f & ~URATH_ON_BALCONY) | URATH_COMING_DOWN;
        evil_urath_leap_flags = f;
        evil_urath_leap_start_y = owner->pos.y;
    }
    evil_urath_leap_height = owner->pos.y;
    evil_urath_pick_teleport_point();
    set_mode(AI_STATE_COMBAT, AIS_URATH_LEAP_RISE);
}

// 0x4c4870  Confidence: likely (the speed formulas follow the x87 order). The boss rises 2 units per second
// (to 2 above the target, or above the floor it left) while it turns to the target. Going up, the
// horizontal speed covers the free distance (to the first wall, less 3) in the time of the remaining rise,
// at most 3 (1.5 without a wall). Coming down it waits for the full rise; then speed 3 and a fall speed
// matched to the distance (2 or 1 without room or a wall).
void ai_state::evil_urath_ai_leap_rise()
{
    float top;
    float dist;
    float speed;
    float a;
    float b;
    bool reached;

    if (evil_urath_leap_flags & URATH_GOING_UP) {
        top = evil_urath_leap_target->pos.y + 2.0f;
        evil_urath_leap_height = reported_frametime + reported_frametime + evil_urath_leap_height;
        if (evil_urath_leap_height > top)
            evil_urath_leap_height = top;
        if (turn_towards(&evil_urath_leap_target->pos, 0))
            return;
        if (evil_urath_leap_check_path(owner, evil_urath_leap_target->layer, &dist) == true) {
            speed = (dist - 3.0f) / ((top - evil_urath_leap_height) * 0.5f);
            if (speed < 3.0f)
                evil_urath_leap_rise_speed = speed;
            else
                evil_urath_leap_rise_speed = 3.0f;
        } else {
            evil_urath_leap_rise_speed = 1.5f;
        }
        set_mode(AI_STATE_COMBAT, AIS_URATH_LEAP_TRAVEL);
        return;
    }
    top = evil_urath_leap_start_y + 2.0f;
    reached = false;
    evil_urath_leap_height = reported_frametime + reported_frametime + evil_urath_leap_height;
    if (evil_urath_leap_height > top) {
        evil_urath_leap_height = top;
        reached = true;
    }
    if (turn_towards(&evil_urath_leap_target->pos, 0))
        return;
    if (reached != true)
        return;
    if (evil_urath_leap_check_path(owner, owner->layer, &dist) == true) {
        dist += 3.0f;
        vector2 d;
        d.x = evil_urath_leap_target->pos.x - owner->pos.x;
        d.y = evil_urath_leap_target->pos.z - owner->pos.z;
        evil_urath_leap_peak_y = d.len() - dist;
        a = evil_urath_leap_peak_y * (1.0f / 3.0f);
        b = (owner->pos.y - (evil_urath_leap_target->pos.y + 2.0f)) * 0.5f;
        if (b > a) {
            evil_urath_leap_rise_speed = 3.0f;
            evil_urath_leap_fall_speed = a / b + a / b;
        } else {
            evil_urath_leap_rise_speed = 3.0f;
            evil_urath_leap_fall_speed = 2.0f;
        }
    } else {
        evil_urath_leap_rise_speed = 3.0f;
        evil_urath_leap_fall_speed = 1.0f;
    }
    set_mode(AI_STATE_COMBAT, AIS_URATH_LEAP_TRAVEL);
}

// 0x4c4ad0  Confidence: likely. Forward at the horizontal speed; going up the height keeps rising to its
// top, coming down it falls (at the fall speed, not below 2 above the target) once the remaining distance
// is under leap_peak_y. Past the target (behind the facing) the landing starts.
void ai_state::evil_urath_ai_leap_travel()
{
    float top;
    float step;
    float left;

    if (evil_urath_leap_flags & URATH_GOING_UP) {
        top = evil_urath_leap_target->pos.y + 2.0f;
        evil_urath_leap_height = reported_frametime + reported_frametime + evil_urath_leap_height;
        if (evil_urath_leap_height > top)
            evil_urath_leap_height = top;
        step = evil_urath_leap_rise_speed * reported_frametime;
        owner->pos.x += step * owner->orient.fvec.x;
        owner->pos.y += step * owner->orient.fvec.y;
        owner->pos.z += step * owner->orient.fvec.z;
        if ((evil_urath_leap_target->pos.z - owner->pos.z) * owner->orient.fvec.z
            + (evil_urath_leap_target->pos.y - owner->pos.y) * owner->orient.fvec.y
            + (evil_urath_leap_target->pos.x - owner->pos.x) * owner->orient.fvec.x < 0.0f)
            set_mode(AI_STATE_COMBAT, AIS_URATH_LEAP_LAND);
        return;
    }
    vector2 d;
    d.x = evil_urath_leap_target->pos.x - owner->pos.x;
    d.y = evil_urath_leap_target->pos.z - owner->pos.z;
    left = d.len();
    step = evil_urath_leap_rise_speed * reported_frametime;
    owner->pos.x += step * owner->orient.fvec.x;
    owner->pos.y += step * owner->orient.fvec.y;
    owner->pos.z += step * owner->orient.fvec.z;
    if (left < evil_urath_leap_peak_y) {
        top = evil_urath_leap_target->pos.y + 2.0f;
        evil_urath_leap_height = evil_urath_leap_height - evil_urath_leap_fall_speed * reported_frametime;
        if (evil_urath_leap_height < top)
            evil_urath_leap_height = top;
        if ((evil_urath_leap_target->pos.z - owner->pos.z) * owner->orient.fvec.z
            + (evil_urath_leap_target->pos.y - owner->pos.y) * owner->orient.fvec.y
            + (evil_urath_leap_target->pos.x - owner->pos.x) * owner->orient.fvec.x < 0.0f)
            set_mode(AI_STATE_COMBAT, AIS_URATH_LEAP_LAND);
    }
}

// 0x4c4ca0  Confidence: likely. Sinks 2 units per second to the target's height while turning to the
// navpoint's heading; then the boss is on the floor or a balcony again (sub-mode -1).
void ai_state::evil_urath_ai_leap_land()
{
    bool reached = true;
    float floor_y = evil_urath_leap_target->pos.y;
    matrix m;

    evil_urath_leap_height = evil_urath_leap_height - (reported_frametime + reported_frametime);
    if (evil_urath_leap_height < floor_y)
        evil_urath_leap_height = floor_y;
    else
        reached = false;
    m.set_from_angles(0.0f, 0.0f, evil_urath_leap_target->heading);
    vector3 facing(m.fvec.x + evil_urath_leap_target->pos.x, m.fvec.y + evil_urath_leap_target->pos.y,
                   m.fvec.z + evil_urath_leap_target->pos.z);
    if (turn_towards(&facing, 0) == true)
        return;
    if (reached != true)
        return;
    owner->flags &= ~CF_URATH_LEAPING;
    owner->layer = evil_urath_leap_target->layer;
    evil_urath_leap_height = 0.0f;
    owner->find_ground();
    if (evil_urath_leap_flags & URATH_COMING_DOWN)
        evil_urath_leap_flags = (evil_urath_leap_flags & ~URATH_COMING_DOWN) | URATH_ON_FLOOR;
    else
        evil_urath_leap_flags = (evil_urath_leap_flags & ~URATH_GOING_UP) | URATH_ON_BALCONY;
    evil_urath_leap_target = NULL;
    set_mode(AI_STATE_COMBAT, -1);
}

// 0x4c4db0  Confidence: likely (the HP band ladder shares the leap tail; the spell switch is a table of 7,
// at 0x4c50ec). HP 80-60 % and 40-20 %: on a balcony, 60-40 % and below 20 %: on the floor; otherwise a leap.
// Then a spell at the target when it is within 15 and 7 in height: seven spells 0x55-0x5b weighted 1/7 each,
// changed by the target flags (bosses.md "Attack choice"); a failed pick falls back to rand() % 6, which
// never gives the seventh.
int ai_state::evil_urath_ai_select_action()
{
    boss_target list[BOSS_MAX_THREATS];
    float weights[7];
    int spell;
    Character *target;
    Character *t;
    bool timer_up;
    int count;
    int index;
    boss_target *e;
    float frac;

    if (evil_urath_casting_spell != -1 || submode == AIS_URATH_LEAP_START || submode == AIS_URATH_LEAP_RISE
        || submode == AIS_URATH_LEAP_TRAVEL || submode == AIS_URATH_LEAP_LAND)
        return 5;
    spell = -1;
    target = living_from_handle(evil_urath_target_handle);
    if (target && (target->flags & CF_HIDDEN))
        target = NULL;
    timer_up = evil_urath_attack_timer.elapsed();
    count = boss_target_list_build(owner, list, evil_urath_threat_table);
    if (timer_up || !target) {
        evil_urath_target_handle = boss_target_list_pick(list, count, evil_urath_threat_table,
                                                         evil_urath_target_handle, false, 0);
        if (evil_urath_target_handle == -1)
            return 0;
        living_from_handle(evil_urath_target_handle);
        evil_urath_set_attack_timer(5000, 15000);
    }
    frac = owner->hp / (float)owner->get_max_stat(0);
    if (frac < 0.2f) {
        if (!(evil_urath_leap_flags & URATH_ON_FLOOR))
            goto leap;
    } else if (frac < 0.4f) {
        if (!(evil_urath_leap_flags & URATH_ON_BALCONY))
            goto leap;
    } else if (frac < 0.6f) {
        if (!(evil_urath_leap_flags & URATH_ON_FLOOR))
            goto leap;
    } else if (frac < 0.8f) {
        if (!(evil_urath_leap_flags & URATH_ON_BALCONY))
            goto leap;
    }
    index = boss_target_list_find_b(list, count, evil_urath_target_handle);
    if (index == -1)
        return 4;
    if (list[index].dist > 15.0f)
        return 4;
    t = living_from_handle(evil_urath_target_handle);
    if (t == NULL || (t->flags & CF_HIDDEN))
        return 0;
    if ((float)fabs(owner->pos.y - t->pos.y) > 7.0f)
        return 4;
    e = &list[index];
    weights[0] = 1.0f / 7.0f;
    if (e->flags & 2)
        weights[0] = 0.1f;
    weights[1] = 1.0f / 7.0f;
    if (e->flags & 4)
        weights[1] = 0.2f;
    else if (e->flags & 1)
        weights[1] = 0.8571429f;
    weights[2] = 1.0f / 7.0f;
    weights[3] = 1.0f / 7.0f;
    if (e->flags & 8)
        weights[3] = 0.2f;
    weights[4] = 1.0f / 7.0f;
    if (e->flags & 0x10)
        weights[4] = 0.2f;
    weights[5] = 1.0f / 7.0f;
    if (e->flags & 0x20)
        weights[5] = 0.2f;
    weights[6] = 1.0f / 7.0f;
    if (e->flags & 0x40)
        weights[6] = 1.0f;
    index = rand_weighted_index(weights, 7);
    if (index == -1)
        index = rand() % 6;
    switch (index) {
    case 0:
        spell = 0x55;
        break;
    case 1:
        spell = 0x56;
        break;
    case 2:
        spell = 0x57;
        break;
    case 3:
        spell = 0x58;
        break;
    case 4:
        spell = 0x59;
        break;
    case 5:
        spell = 0x5a;
        break;
    case 6:
        spell = 0x5b;
        break;
    }
    evil_urath_cast_spell(spell, t);
    return 1;
leap:
    evil_urath_pending_mode = AIS_URATH_LEAP_START;
    return 1;
}

// 0x4c5110  Confidence: exact.
void evil_urath_set_attack_timer(int min_ms, int max_ms)
{
    evil_urath_attack_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}
