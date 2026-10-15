// ai.cpp - Sum.exe v1.40, code 0x471ff0-0x475178 (module "ai.cpp" in sum.symbols for 0x471ff0-0x475087;
// directory Engine\ai from the pathfinding.cpp path literal). Written from the Sum.exe disassembly
// (tools/asm.mjs fn; the jump tables and the .data / .rdata values read from Summoner/Sum.exe).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other optimised units (no frame pointer, 0x90 padding,
// arguments' stack slots reused for locals).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: init_empty_472000_thunk (0x471ff0, static initialiser entry 218; mp_handle_kick_pl 0x471f90
// ends the multiplayer unit before it) to cast_effect_track_count (0x475160-0x475178). The three
// cast_effect_* functions after ai_can_be_knocked_down are module "-" in sum.symbols; their table
// 0x22e4950 lies inside this unit's .bss and their .rdata constant 1/15 (0x581a08) follows this unit's
// 0.707 / 0.0025 / PI/8 (0x5819fc-0x581a04), so they are written here (D1905). mp_find_player_by_addr
// (0x475180) starts the next unit.
//
// Profiling: ai_frame opens an EH frame whose only unwind action is the empty destructor 0x434650
// (profile_label, vsdk/os/profile.h; EH record 0x583d08, unwind 0x57c500 on ebp-0x1c), as in
// pathfinding.cpp; the label text is not in the binary, the function name is used (unsure). The
// timer_get(1000000) deltas feed the ai_prof counters, averaged every 500 ms by ai_do_frame.
//
// Data: .data 0x59ad60-0x59b7c7 (ai_goal_next_id 0x59b7c8 and "WARNING: unknown service request" belong
// to the goal unit after this one): the three name tables, the species style table and its count, the
// console variables, then the literals (the tables' strings in reverse order, then the functions' in
// order of use). "TRUE" / "FALSE", "%s is %s\n" and the toggle usage format are pooled (0x588c34-0x588c50),
// "" (0x5bd0d0) and the goal names "attack", "protect", "backstab", "sneak", "hide" too.
// .bss 0x22e47d0-0x22e49b7 (between multitrade.cpp's 0x22e47cb and camera_occluders 0x22e49b8).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/effects/fx.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/profile.h"
#include "vsdk/math/vector.h"
#include "vsdk/gr/gr.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/geom/effect_mem.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/localize/localize.h"
#include "levelscripts/level_scripts.h"
#include "summoner/summoner.h"
#include "summoner/gametarget.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "summoner/multi/multi.h"

#define CHAR_TYPE_MONSTER 5           // obj +0xc: the types ai_call_for_help / ai_state_save test (4, 5)
#define CHAR_TYPE_NPC     4
#define CHAR_TYPE_PARTY   7
#define CHAR_TYPE_BOSS    9

// Sub-modes used here (AIS_* names from ai_substate_names where the table lines up, D1906).
#define AIS_ENTER_HIDING    0x09
#define AIS_LEAVE_HIDING    0x0b
#define AIS_ENTER_SNEAKING  0x0c
#define AIS_LEAVE_SNEAKING  0x0d
#define AIS_TRANSITION      0x0e
#define AIS_CLOSE_COMBAT    0x0f
#define AIS_RANGED_COMBAT   0x10
#define AIS_BACKSTAB        0x11
#define AIS_CHOOSE_SPELL    0x15
#define AIS_GET_CLOSE       0x16
#define AIS_GET_FAR         0x17
#define AIS_HIDE_UNTIL_VULNERABLE 0x1d
#define AIS_TRIPPED_FALL    0x2c      // the table is five entries short from here on (D1906)
#define AIS_TRIPPED_DOWN    0x2d      // knocked down
#define AIS_TRIPPED_UP      0x2e
#define AIS_PUSHED          0x2f
#define AIS_STUNNED         0x31
#define AIS_CHARGE          0x3f      // ai_charge_target (no name in the table)

#define AI_DIRECT_PRIORITY  999999    // +0x38 for direct orders (0xf423f)
#define AI_PROF_PERIOD      500       // ms between two averages of the profiler counters

// ---- .data (0x59ad60-0x59b7c7) ---------------------------------------------------------------------
char *ai_state_names[NUM_AI_STATES] = {                       // 0x59ad60
    "AI_STATE_INACTIVE", "AI_STATE_IDLE", "AI_STATE_TRAVELING", "AI_STATE_COMBAT", "AI_STATE_CAST_SPELL",
    "AI_STATE_PATROL", "AI_STATE_SUMMONING", "AI_STATE_BANISHING", "AI_STATE_DISABLED", "AI_STATE_HIDING",
    "AI_STATE_SNEAKING", "AI_STATE_MOUNT_TRAVEL", "AI_STATE_MOUNT_IDLE",
};
char *ai_substate_names[65] = {                               // 0x59ad94 (55 names, 10 NULL)
    "AIS_PATH_TRAVELING", "AIS_DIRECT_TRAVELING", "AIS_TURN_TO_TARGET", "AIS_FOLLOW", "AIS_FOLLOW_WAITUP",
    "AIS_FOLLOW_STOPPED", "AIS_GOTO_NAVPOINT", "AIS_WALKTO_NAVPOINT", "AIS_WANDER", "AIS_ENTER_HIDING",
    "AIS_HIDING", "AIS_LEAVE_HIDING", "AIS_ENTER_SNEAKING", "AIS_LEAVE_SNEAKING", "AIS_TRANSITION",
    "AIS_CLOSE_COMBAT", "AIS_RANGED_COMBAT", "AIS_BACKSTAB", "AIS_EVADE", "AIS_RUN_AWAY", "AIS_GUARD",
    "AIS_CHOOSE_SPELL", "AIS_GET_CLOSE", "AIS_GET_FAR", "AIS_GET_BEHIND_CLOSE", "AIS_GET_BEHIND_FAR",
    "AIS_GET_IN_FRONT_CLOSE", "AIS_GET_IN_FRONT_FAR", "AIS_SNEAK_TO_POSITION", "AIS_HIDE_UNTIL_VULNERABLE",
    "AIS_RIDER_CHARGE_SETUP", "AIS_RIDER_CHARGE", "AIS_SAFETY", "AIS_ATTACK_TRAVEL", "AIS_SPELL",
    "AIS_TARGETED_SPELL", "AIS_LOCATION_SPELL", "AIS_PATROL_WAYPOINTS", "AIS_GET_OUT_OF_WAY",
    "AIS_TRIPPED_FALL", "AIS_TRIPPED_DOWN", "AIS_TRIPPED_UP", "AIS_PUSHED", "AIS_PUSH_STUNNED",
    "AIS_STUNNED", "AIS_BEAM_ATTACK", "AIS_TELEPORT_OUT", "AIS_TELEPORT_IN", "AIS_GRAB",
    "AIS_GRAB_APPROACH", "AIS_THROW", "AIS_ILLUSION_OUT", "AIS_ILLUSION_IN", "AIS_BURNING_START",
    "AIS_BURNING_END",
};
char *ai_goal_type_names[18] = {                              // 0x59ae98
    "unused", "attack", "follow", "cast_spell", "cast_targeted_spell", "cast_location_spell",
    "switch_combat_mode", "switch_normal_mode", "protect", "pathfind", "turn_to", "run_away",
    "goto_navpoint", "wander", "backstab", "sneak", "hide", "pickup item",
};
ai_species_style ai_species_styles[14] = {                    // 0x59aee0
    { 0x9f, 5 }, { 0xa3, 4 }, { 0xa4, 5 }, { 0xa6, 5 }, { 0x45, 4 }, { 0x41, 4 }, { 0x3d, 4 },
    { 0x2c, 4 }, { 0x3c, 5 }, { 0xb9, 4 }, { 0xbd, 5 }, { 0xbe, 5 }, { 0x98, 5 }, { 0xd4, 5 },
};
int num_ai_species_styles = sizeof(ai_species_styles) / sizeof(ai_species_styles[0]);   // 0x59af50
float slowdown_scaler = 4.0f;                                 // 0x59af54
bool Ai_path_wandering_enabled = true;                        // 0x59af58
bool ai_path_flag_59af59 = true;                              // 0x59af59
int ai_frame_update_rate = 8;                                 // 0x59af5c
int Ai_attack_point = -1;                                     // 0x59af60
int Ai_offense_point = -1;                                    // 0x59af64

// ---- .bss (0x22e47d0-0x22e49b7), the objects without initialisers in address order -------------------
unsigned int ai_move_flags;                                   // 0x22e48cc
cast_effect_track cast_effect_tracks[MAX_CAST_EFFECT_TRACKS]; // 0x22e4950
bool Ai_paused;                                               // 0x22e499c
bool ai_do_frame_active;                                      // 0x22e499d

// The dynamically initialised objects, in initialiser-table order (entries 218-221).
// The two vector3 objects (empty inline constructor); which initialiser is which is not visible.
// 0x471ff0 init_empty_472000_thunk (jmp, static initialiser entry 218). Confidence: unsure (the assignment).
// 0x472000 init_empty_472000 (a bare ret). Confidence: unsure (the assignment).
// 0x472010 init_empty_472020_thunk (jmp, entry 219). Confidence: unsure (the assignment).
// 0x472020 init_empty_472020 (a bare ret). Confidence: unsure (the assignment).
vector3 ai_directions[16];                                    // 0x22e47f0
vector3 ai_move_dir;                                          // 0x22e4940
// 0x472030 init_realtimetimers_22e48e0_thunk (jmp, entry 220). Confidence: likely.
// 0x472040 init_realtimetimers_22e48e0: the compiler-generated constructor of ai_profile (two
//          RealtimeTimer calls, D1900). Confidence: likely.
ai_profile ai_prof;                                           // 0x22e48e0
// 0x472060 init_strptrs_22e49a0_thunk (jmp, entry 221). Confidence: likely.
// 0x472070 init_strptrs_22e49a0 (the six slot addresses; as new_status.cpp's slot tables). Confidence: likely.
char **ai_combat_style_names[6] = {                           // 0x22e49a0
    XSTR_SLOT("Melee", 0), XSTR_SLOT("Support", 1), XSTR_SLOT("Range", 2), XSTR_SLOT("Healer", 3),
    XSTR_SLOT("Caster", 4), XSTR_SLOT("Healer/Caster", 5),
};

// ---- console commands ------------------------------------------------------------------------------
// 0x4720b0 init_ccmd_ai_frame_update_rate_thunk (jmp, static initialiser entry 222). Confidence: exact.
// 0x4720c0 init_ccmd_ai_frame_update_rate (console_command constructor). Confidence: exact.
console_command ccmd_ai_frame_update_rate("ai_frame_update_rate",
    "Set the rate at which AI is updated in frames for offscreen creatures", cmd_ai_frame_update_rate);   // 0x22e4990

// 0x4720e0  Confidence: exact. Hand-written (status before help, unlike the macro commands). The value
// is never read anywhere (NOTES.md).
void cmd_ai_frame_update_rate()
{
    if (calling_a_command_function) {
        process_command(ARG_INT);
        if (command_argument_int >= 1 && command_argument_int <= 30)
            ai_frame_update_rate = command_argument_int;
        else
            print_to_console("Invalid range for value\n", 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Ai frame update rate is %d\n", ai_frame_update_rate);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: ai_frame_update_rate <value>\n", 0);
}

// 0x472160 init_ccmd_slowdown_scaler_thunk (jmp, static initialiser entry 223). Confidence: exact.
// 0x472170 init_ccmd_slowdown_scaler (console_command constructor). Confidence: exact.
console_command ccmd_slowdown_scaler("slowdown_scaler", "Set the slowdown scaler", cmd_slowdown_scaler);   // 0x22e47e0

// 0x472190  Confidence: exact.
void cmd_slowdown_scaler()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        slowdown_scaler = command_argument_float;
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Slowdown scaler is %.3f\n", slowdown_scaler);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: slowdown_scaler <value>\n", 0);
}

// ---- ai_state --------------------------------------------------------------------------------------
// 0x472200  Confidence: likely (the team / type test is a branch-free AND: `sete dl; sete al; test eax,
// edx`, read as && of two side-effect-free tests). The alert sound, then for monsters every other
// monster of the team that is not fighting and has no target, within the detection range, travels to
// the caller (character_ai_add_goal9, priority 1000).
void ai_state::call_for_help()
{
    Character *c;
    int team;

    if (owner->info->alert_sound.handle == -1)
        return;
    sound_play_3d_at(owner->info->alert_sound.handle, &owner->pos);
    if (owner->type != CHAR_TYPE_MONSTER)
        return;
    team = owner->get_team();
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c == owner)
            continue;
        if (c->get_team() == team && c->type == CHAR_TYPE_MONSTER) {
            if (!c->ai.is_mode(AI_STATE_COMBAT, -1) && c->ai.target == -1 &&
                distance(&owner->pos, &c->pos) < owner->info->detection_range)
                c->ai_add_goal9(&owner->pos, owner->layer, 1000, 1);
        }
    }
}

// 0x4722f0  Confidence: exact. Only the GameTimer members have constructors (the attacker slots' loop
// is the inline constructor of the anonymous slot struct).
ai_state::ai_state()
{
}

// 0x472380  Confidence: exact (the attack point list is a memset with 0xff: `rep stosd` of -1).
void ai_state::init()
{
    int i;

    owner = NULL;
    set_mode(-1, -1);
    combat_style = -1;
    timer3c.SetTimeout(0);
    perception_timer.SetRandomTimeout(0, 50);
    timer224.SetTimeout(0);
    path = NULL;
    _unk220 = -1;
    flags = 0;
    for (i = 0; i < 5; i++) {
        goals[i].type = 0;
        goals[i].priority = -1;
        goals[i].flags = 0;
        goals[i].path = NULL;
    }
    handle_slot = -1;
    group = NULL;
    _unk1ac = 0;
    _unk7c = -1;
    for (i = 0; i < 5; i++)
        attackers[i].handle = -1;
    memset(attack_point_users, -1, sizeof(attack_point_users));
}

// 0x472420  Confidence: exact.
void ai_state::shutdown()
{
    ai_path_free(path);
    if (owner)
        boss_ai_unload();
}

// 0x472450  Confidence: likely (the species loop's form; the owner is reloaded for every store as in the
// binary, so each statement goes through owner).
void ai_state::reset()
{
    int i;

    if (owner->type == CHAR_TYPE_BOSS || owner->info->species == SPECIES_LUMINAR)
        boss_ai_load();
    go_idle();
    goals_clear();
    for (i = 0; i < num_ai_species_styles; i++) {
        if (owner->info->species == ai_species_styles[i].species) {
            combat_style = ai_species_styles[i].style;
            break;
        }
    }
    for (i = 0; i < 5; i++) {
        attackers[i].handle = -1;
        attackers[i].timer.reset();
    }
    memset(attack_point_users, -1, sizeof(attack_point_users));
    ai_path_free(path);
    path = NULL;
    _unk204 = 0;
    owner->loco_blend = 0.0f;
    owner->target_speed = 0.0f;
    owner->last_pos = owner->pos;
    owner->last_orient = owner->orient;
    owner->forward_speed = 0.0f;
    owner->velocity.zero();
    owner->frametime = 0.0f;
    owner->cur_move = NULL;
    owner->cur_anim = -1;
    owner->stance_blend = 1.0f;
    owner->update_locomotion_anim();
    cur_goal = 0;
    target = -1;
    prev_target = -1;
    _unk198 = -1;
    _unk88 = -1;
    timer19c.SetTimeout(0);
}

// 0x4725e0  Confidence: exact. Party members only (party_level_reset).
void ai_state::reset_party_member()
{
    if (owner->type != CHAR_TYPE_PARTY)
        return;
    ai_path_free(path);
    path = NULL;
    cur_goal = 0;
    goals_clear();
    reset();
    owner->start_navpoint = NULL;
    owner->stance_blend = 1.0f;
    owner->prop_tag = owner->default_prop_tag;
    owner->flags &= ~0x1;
    owner->flags &= ~0x100000;
    owner->flags &= ~0x400;
    owner->ground_face = -1;
    owner->ground_lobj = -1;
}

// 0x4726a0  Confidence: exact.
int ai_state::count_attack_slots_used()
{
    int n = 0;
    int i;

    for (i = 0; i < 5; i++)
        if (living_from_handle(attackers[i].handle))
            n++;
    return n;
}

// 0x4726d0  Confidence: exact (jump table 0x472768, five cases). Unit vectors 72 degrees apart.
void ai_state::attack_slot_direction(vector3 *out, int slot)
{
    switch (slot) {
    case 0: out->x = 1.0f;       out->y = 0.0f; out->z = 0.0f;       break;
    case 1: out->x = 0.3090169f; out->y = 0.0f; out->z = 0.9510565f; break;
    case 2: out->x = -0.8090169f; out->y = 0.0f; out->z = 0.5877852f; break;
    case 3: out->x = -0.8090169f; out->y = 0.0f; out->z = -0.5877852f; break;
    case 4: out->x = 0.3090169f; out->y = 0.0f; out->z = -0.9510565f; break;
    }
}

// 0x472780  Confidence: likely (the dot product's operand order). The slot whose direction best matches
// the attacker's bearing from the owner; it does not look at whether the slot is taken (NOTES.md).
int ai_state::pick_attack_slot(Character *attacker)
{
    vector3 me = owner->get_center();
    vector3 him = attacker->get_center();
    vector3 dir, s;
    float best = -999.9f;
    float dot;
    int best_slot = 0;
    int i;

    dir.x = him.x - me.x;
    dir.y = him.y - me.y;
    dir.z = him.z - me.z;
    for (i = 0; i < 5; i++) {
        attack_slot_direction(&s, i);
        dot = s.z * dir.z + s.y * dir.y + dir.x * s.x;
        if (dot > best) {
            best = dot;
            best_slot = i;
        }
    }
    return best_slot;
}

// 0x472840  Confidence: likely (the best-slot search is pick_attack_slot's loop expanded with -1 as
// the starting slot). Riders are skipped. The attacker leaves its old slot (keeping the shorter time);
// a taken slot is only replaced when its occupant has more time left than ms.
void ai_state::claim_attack_slot(Character *self, Character *attacker, int ms, int slot)
{
    vector3 me, him, dir, s;
    float best, dot;
    int i, rem, best_slot;

    if (attacker->get_rider())
        return;
    for (i = 0; i < 5; i++) {
        if (attackers[i].handle == attacker->handle) {
            attackers[i].handle = -1;
            rem = attackers[i].timer.GetRemainingTime();
            if (rem >= 0 && rem < ms)
                ms = rem;
        }
    }
    if (slot == -1) {
        best_slot = -1;
        best = -999.9f;
        me = self->get_center();
        him = attacker->get_center();
        dir.x = him.x - me.x;
        dir.y = him.y - me.y;
        dir.z = him.z - me.z;
        for (i = 0; i < 5; i++) {
            attack_slot_direction(&s, i);
            dot = s.z * dir.z + s.y * dir.y + dir.x * s.x;
            if (dot > best) {
                best = dot;
                best_slot = i;
            }
        }
        slot = best_slot;
    }
    if (slot < 0 || slot >= 5)
        return;
    if (living_from_handle(attackers[slot].handle) && attackers[slot].timer.GetRemainingTime() <= ms)
        return;
    attackers[slot].handle = attacker->handle;
    attackers[slot].timer.SetTimeout(ms);
}

// 0x4729b0  Confidence: likely (the temporaries' order). travel_dest = the owner's position moved
// offset->z along the horizontal direction to pos and offset->x to its right-hand side.
void ai_state::offset_point_sideways(const vector3 *pos, const vector3 *offset)
{
    vector3 d(pos->x - owner->pos.x, 0.0f, pos->z - owner->pos.z);
    vector3 side;

    d.normalize();
    side = vector3(d.z, 0.0f, -d.x) * offset->x;
    travel_dest = owner->pos + d * offset->z - side;
}

// ---- the per-entity frame --------------------------------------------------------------------------
// 0x472a90  Confidence: likely (cdecl on the character, D1901; jump table 0x472e40: 0 / 12 share the
// inactive body, 5, 7 and 11 do nothing; the idle body falls into it). The movement step, the
// multiplayer travel flags and the death check follow the mode handler.
void ai_frame(Character *c)
{
    profile_label label("ai_frame");
    Player *p = player_from_handle(c->handle);
    ai_state *ai;
    int t_start, t0, t1, t2, handle;
    int mode;

    if ((game_flags & GF_MULTIPLAYER) && c->type == CHAR_TYPE_PARTY) {
        if (p->mp_owner->flags & MP_PF_TRAVELING)
            p->mp_owner->flags |= MP_PF_WAS_TRAVELING;
        else
            p->mp_owner->flags &= ~MP_PF_WAS_TRAVELING;
        p->mp_owner->flags &= ~MP_PF_TRAVELING;
    }
    t_start = timer_get(1000000);
    handle = c->handle;
    ai = &c->ai;
    ai_move_flags = 0;
    if (c->ai_suspended())
        return;
    c->frametime += (cutscene_active_count && !(c->flags & 0x800)) ? cutscene_frametime : reported_frametime;
    if (ai_update_perception(c) == true)
        return;
    if ((ai->owner->flags & 0x400) && ai->timer44.elapsed())
        ai->add_sneak();
    if ((c->flags & CF_HIDDEN) && c->type != CHAR_TYPE_BOSS) {
        ai->go_idle();
        ai->go_inactive();
    }
    if ((game_flags & GF_SINGLE_PLAYER) || character_is_locally_controlled(c))
        ai->think();
    mode = ai->get_mode();
    if (c->type == CHAR_TYPE_BOSS)
        ai->boss_ai_check_stage();
    if (c->mount_link != -1 && !living_from_handle(c->mount_link))
        c->mount_link = -1;

    switch (mode) {
    case AI_STATE_IDLE:
        if (c->type == CHAR_TYPE_BOSS) {
            ai->boss_ai_idle_dispatch();
        } else if ((c->flags & CF_SUMMON) && (ai->flags & 0x200) && ai->disable_timer.elapsed()) {
            ai->begin_emerge();
            ai->disable_timer.invalidate();
            ai->flags &= ~0x200;
        }
        // fall through
    case AI_STATE_INACTIVE:
    case AI_STATE_MOUNT_IDLE:
        if (character_is_locally_controlled(c))
            c->set_6ac(0.0f);
        break;
    case AI_STATE_HIDING:
        ai->mode_hiding_frame();
        break;
    case AI_STATE_SNEAKING:
        ai->mode_sneaking_frame();
        break;
    case AI_STATE_TRAVELING:
        ai->mode_traveling_frame();
        break;
    case AI_STATE_COMBAT:
        ai->mode_combat_frame();
        break;
    case AI_STATE_CAST_SPELL:
        ai->mode_cast_spell_frame();
        break;
    case AI_STATE_SUMMONING:
        ai->summon_emerge_update();
        break;
    case AI_STATE_DISABLED:
        ai->mode_disabled_frame();
        break;
    }
    if ((c->flags & 0x400) && !ai->is_mode(AI_STATE_SNEAKING, -1))
        ai->mode_sneaking_frame();

    t0 = timer_get(1000000);
    if (ai->flags & 0x10000000) {
        c->slowdown_rate *= slowdown_scaler;
        c->ai_move_frame(ai_move_flags, &ai_move_dir);
    } else {
        c->ai_move_frame(ai_move_flags, NULL);
    }
    t1 = timer_get(1000000);

    if ((game_flags & GF_MULTIPLAYER) && c->type == CHAR_TYPE_PARTY) {
        if (ai->is_traveling_sub1())
            p->mp_owner->flags |= MP_PF_TRAVELING;
        else
            p->mp_owner->flags &= ~MP_PF_TRAVELING;
        if (c == cur_character) {
            if (mp_local_player->flags & MP_PF_TRAVELING) {
                if (!(mp_local_player->flags & MP_PF_WAS_TRAVELING)) {
                    mp_send_swi_2dir();
                    mp_local_player->move_update_timer.SetTimeout(100);
                }
                if (mp_local_player->move_update_timer.elapsed()) {
                    mp_send_obj_update(cur_character, 0);
                    mp_local_player->move_update_timer.SetTimeout(100);
                }
            } else if (mp_local_player->flags & MP_PF_WAS_TRAVELING) {
                mp_send_swi_ndir();
            }
        }
    }
    c->on_death();
    t2 = timer_get(1000000);
    ai_prof_frame_us += t1 - t0;
    if (handle == world_mouse_target_handle) {
        ai_prof_frame_b_us += t2 - t_start;
        ai_prof_frame_c_us += t1 - t0;
    }
}

// 0x472e80  Confidence: likely (the early returns' order). True skips the rest of the character's AI
// this frame (off-screen or inactive). In multiplayer the host runs only idle monsters' AI on a
// 500-750 ms timer; offline, characters outside the view (sphere test with 5 x the bounding radius) or
// not active skip it, and the frame time is reset whenever the AI runs.
bool ai_update_perception(Character *c)
{
    vector3 centre;
    float radius;
    unsigned int f;
    bool culled;

    c->ai.flags |= 8;
    f = c->ai.flags;
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && (mp_local_player->flags & 1)) {
        if (c->type == CHAR_TYPE_PARTY) {
            c->frametime = reported_frametime;
            return false;
        }
        if (!c->ai.is_mode(AI_STATE_IDLE, -1)) {
            c->frametime = reported_frametime;
            return false;
        }
        if (!(c->ai.flags & 0x40)) {
            c->frametime = reported_frametime;
            return false;
        }
        if (!c->ai.timer19c.elapsed())
            goto skip;
        c->ai.timer19c.SetRandomTimeout(500, 750);
        c->frametime = reported_frametime;
        return false;
    }
    if ((game_flags & GF_CUTSCENE) && c->cutscene_object >= 0)
        return false;
    if (c->obj_flags & 2) {
        if (f & 0x40)
            goto visible;
        return false;
    }
    if (c->ai.is_mode(AI_STATE_COMBAT, -1))
        return false;
    if (!(c->obj_flags & 1) && !(c->flags & CF_SUMMON))
        return true;
    if (c->type == CHAR_TYPE_PARTY)
        return false;
    if (c->get_mount())
        return false;
    if (c->ai.is_combat_e_or_sneak_attack() == true)
        return false;
    if (c->flags & CF_DEAD)
        return false;
    if (c->flags & CF_HIDDEN)
        return true;
    if (c->type == CHAR_TYPE_BOSS)
        return false;
    if (!c->ai.timer19c.elapsed())
        goto skip;
    c->ai.timer19c.SetRandomTimeout(500, 750);
    if (!(c->ai.flags & 0x40))
        return false;
    c->instance->get_bounding_sphere(&centre, &radius);
    centre.unrotate(c->orient);
    centre.x += c->pos.x;
    centre.y += c->pos.y;
    centre.z += c->pos.z;
    radius *= 5.0f;
    culled = gr_cull_sphere(&centre, radius, 0);
    f = c->ai.flags;
    if (culled == true) {
        c->ai.flags = f & ~8;
        return true;
    }
visible:
    c->ai.flags = f & ~0x40;
    c->frametime = reported_frametime;
    return false;
skip:
    c->ai.flags &= ~8;
    c->flags &= ~0x18000000;
    return true;
}

// ---- mode tests ------------------------------------------------------------------------------------
// 0x4730c0  Confidence: exact.
bool ai_state::is_traveling_sub0()
{
    return mode == AI_STATE_TRAVELING && submode == 0;
}

// 0x4730e0  Confidence: likely (`mov eax, 1; cmp edx, eax`: the constant is shared with the result).
bool ai_state::is_traveling_sub1()
{
    return mode == AI_STATE_TRAVELING && submode == 1;
}

// 0x473100  Confidence: exact. Goal slot 0 is the follow goal that is running.
bool ai_state::is_following_leader()
{
    return goals[0].type == AI_GOAL_FOLLOW && goals[0].id == cur_goal;
}

// 0x473120  Confidence: exact.
bool ai_state::is_combat_e_or_sneak_attack()
{
    if (mode == AI_STATE_COMBAT && submode == AIS_TRANSITION)
        return true;
    if (mode == AI_STATE_SNEAKING && (submode == AIS_ENTER_SNEAKING || submode == AIS_LEAVE_SNEAKING))
        return true;
    return false;
}

// 0x473150  Confidence: exact.
bool ai_state::is_attacking()
{
    return mode == AI_STATE_COMBAT &&
           (submode == AIS_CHARGE || submode == AIS_CLOSE_COMBAT || submode == 0x1f);
}

// 0x473170  Confidence: exact.
bool ai_state::is_combat_sub10()
{
    return mode == AI_STATE_COMBAT && submode == AIS_RANGED_COMBAT;
}

// 0x473190  Confidence: exact.
bool ai_state::is_combat_sub16_17()
{
    return mode == AI_STATE_COMBAT && (submode == AIS_GET_CLOSE || submode == AIS_GET_FAR);
}

// 0x4731b0  Confidence: exact.
bool ai_state::is_combat_sub16()
{
    return mode == AI_STATE_COMBAT && submode == AIS_GET_CLOSE;
}

// 0x4731d0  Confidence: exact.
bool ai_state::is_casting()
{
    return mode == AI_STATE_CAST_SPELL;
}

// 0x4731e0  Confidence: exact.
bool ai_state::is_combat_sub11()
{
    return mode == AI_STATE_COMBAT && submode == AIS_BACKSTAB;
}

// 0x473200  Confidence: exact.
bool ai_state::is_mode(int m, int sub)
{
    return mode == m && (sub == -1 || submode == sub);
}

// 0x473230 init_ccmd_attack_point_thunk (jmp, static initialiser entry 224). Confidence: exact.
// 0x473240 init_ccmd_attack_point (console_command constructor). Confidence: exact.
console_command ccmd_attack_point("attack_point", "attack point", cmd_attack_point);   // 0x22e4930

// 0x473260  Confidence: likely (the store of the value sits between the two range tests). Picks an
// attack point of the study object; the upper test lets the index equal the point count (NOTES.md).
void cmd_attack_point()
{
    Character *c;

    if (!calling_a_command_function)
        return;
    c = living_from_handle(world_mouse_target_handle);
    if (!c) {
        print_to_console("Error: No study object selected\n", 0);
        return;
    }
    process_command(ARG_INT);
    if (command_argument_int < 0 || (Ai_attack_point = command_argument_int) > c->info->num_target_points)
        Ai_attack_point = -1;
}

// 0x4732d0 init_ccmd_offense_point_thunk (jmp, static initialiser entry 225). Confidence: exact.
// 0x4732e0 init_ccmd_offense_point (console_command constructor). Confidence: exact.
console_command ccmd_offense_point("offense_point", "offense point", cmd_offense_point);   // 0x22e47d0

// 0x473300  Confidence: exact.
void cmd_offense_point()
{
    if (calling_a_command_function) {
        process_command(ARG_INT);
        Ai_offense_point = command_argument_int;
    }
}

// 0x473320  Confidence: likely (byte-indexed jump table 0x4735f0 / 0x4735e0 over sub 0xf..0x1d; the case
// bodies are in binary order). Leaving AI_STATE_HIDING other than through AIS_LEAVE_HIDING is turned
// into AIS_LEAVE_HIDING with the request kept in pending_*; a sneaking owner (flag 0x400) gets the sneak
// goal instead of any mode but traveling, sneaking and idle. The handle slot is released on every change.
int ai_state::set_mode(int new_mode, int new_sub)
{
    Character *t;
    Character *mount;

    if (mode == new_mode && submode == new_sub)
        return 0;
    if (new_mode == AI_STATE_DISABLED && (is_casting() || is_combat_e_or_sneak_attack()))
        return 0;
    prev_mode = mode;
    prev_submode = submode;
    if (mode == AI_STATE_CAST_SPELL && new_mode != mode)
        mode_target = -1;
    handle_slot_release(handle_slot);
    handle_slot = -1;
    mode = new_mode;
    submode = new_sub;
    timer.SetTimeout(0);
    if (new_mode == -1)
        return 1;
    t = living_from_handle(target);
    if (prev_mode == AI_STATE_HIDING && prev_submode != AIS_LEAVE_HIDING && new_mode != AI_STATE_HIDING) {
        pending_submode = new_sub;
        pending_mode = new_mode;
        mode = AI_STATE_HIDING;
        submode = AIS_LEAVE_HIDING;
        flags |= 2;
        if (game_flags & GF_MULTIPLAYER)
            mp_send_swi_hide(owner);
        if (owner->type == CHAR_TYPE_PARTY)
            player_start_skill_cooldown(owner, 0x17);
        return 1;
    }
    if ((owner->flags & 0x400) && new_mode != AI_STATE_TRAVELING && new_mode != AI_STATE_SNEAKING &&
        new_mode != AI_STATE_IDLE) {
        add_sneak();
        return 1;
    }
    if (new_mode == AI_STATE_DISABLED || is_mode(AI_STATE_COMBAT, AIS_TRANSITION))
        flags |= 2;
    else
        flags &= ~2;
    if (prev_mode == AI_STATE_COMBAT && new_mode != AI_STATE_COMBAT) {
        if (t && _unk7c != -1) {
            if (t->info->num_target_points != 0)
                t->ai.attack_point_users[_unk7c] = -1;
            _unk7c = -1;
        }
        flags &= ~0x90;
    }
    if (new_mode == AI_STATE_IDLE) {
        target = -1;
        mode_target = -1;
        priority = 0;
    } else if (new_mode == AI_STATE_COMBAT) {
        if (prev_mode != new_mode && prev_target != -1) {
            target = prev_target;
            prev_target = -1;
        }
        mount = owner->get_mount();
        switch (new_sub) {
        case AIS_GET_CLOSE: case AIS_GET_FAR: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c:
        case AIS_HIDE_UNTIL_VULNERABLE:
            flags |= 4;
            break;
        case AIS_CLOSE_COMBAT:
        case AIS_RANGED_COMBAT:
            owner->set_6ac(0.0f);
            flags &= ~4;
            if (mount)
                mount->ai.go_mount_idle();
            if (t)
                owner->combat.set_target((int)owner, t);
            break;
        case AIS_CHOOSE_SPELL:
            flags &= ~4;
            timer.SetTimeout((int)frand_range(500.0f, 1500.0f));
            break;
        default:
            flags &= ~4;
            break;
        }
    } else if (new_mode == AI_STATE_TRAVELING && new_sub == 6) {
        target = -1;
    }
    path_started = 0;
    path_flags &= ~2;
    return 1;
}

// 0x473600  Confidence: exact.
int ai_state::get_mode()
{
    return mode;
}

// 0x473610  Confidence: exact.
void ai_state::set_18(int style)
{
    combat_style = style;
}

// 0x473620  Confidence: exact. A melee fighter whose target is a boss in its mounted stage fights as
// "Range" (2).
int ai_state::get_combat_style()
{
    Character *t;

    if (combat_style == 0) {
        t = living_from_handle(target);
        if (t && boss_species_mounted_stage(t->info->species))
            return 2;
    }
    return combat_style;
}

// 0x473660  Confidence: exact.
bool ai_state::path_get_destination(vector3 *out)
{
    ai_path *n = path;

    if (!n)
        return false;
    while (n->next)
        n = n->next;
    *out = n->pos;
    return true;
}

// 0x4736a0  Confidence: likely (the || chain's layout). Living entities targeting the owner that fight it
// (close, ranged or positioning sub-modes) or, unless ignore_goals, hold an attack goal on it.
int ai_state::count_engaged_attackers(bool ignore_goals)
{
    Character *c;
    int n = 0;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if ((c->ai.target == owner->handle &&
             (c->ai.is_attacking() || c->ai.is_combat_sub10() || c->ai.is_combat_sub16_17())) ||
            (!ignore_goals && c->ai.goal_find(4, AI_GOAL_ATTACK, 0, 0, owner->handle, 0, 0, AI_NO_SPELL) != -1))
            n++;
    }
    return n;
}

// 0x473750  Confidence: exact.
int ai_state::count_attackers()
{
    Character *c;
    int n = 0;

    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        if (c->ai.target == owner->handle && c->ai.is_attacking())
            n++;
    return n;
}

// 0x4737a0  Confidence: likely (the dot product's operand order, as combat.cpp). An enemy that can be
// backstabbed, within 2 m edge to edge, with the owner behind it (cos 45 degrees).
bool ai_state::can_backstab_target(Character *t)
{
    vector3 d, back;

    if (!owner->is_enemy(t))
        return false;
    if (t->info->flags & CIF_NO_BACKSTAB)
        return false;
    if (!(owner->distance_between_edges(t, false, -1) <= 2.0f))
        return false;
    d = owner->pos - t->pos;
    d.normalize();
    back = -t->orient.fvec;
    if (d.z * back.z + d.y * back.y + d.x * back.x > 0.707f)
        return true;
    return false;
}

// 0x473880  Confidence: exact (the mount's go_idle is a tail jump).
void ai_state::go_inactive()
{
    Character *m;

    flags &= ~0x10000000;
    set_mode(AI_STATE_INACTIVE, -1);
    m = owner->get_mount();
    if (m)
        m->ai.go_idle();
}

// 0x4738c0  Confidence: exact (the recursion on the mount is compiled as a loop: tail-recursion
// elimination with this = &mount->ai).
void ai_state::go_idle()
{
    Character *m;

    owner->set_6ac(0.0f);
    set_mode(AI_STATE_IDLE, -1);
    priority = 0;
    flags &= ~0x10000010;
    m = owner->get_mount();
    if (m)
        m->ai.go_idle();
}

// 0x473940  Confidence: exact.
void ai_state::go_mount_idle()
{
    owner->set_6ac(0.0f);
    set_mode(AI_STATE_MOUNT_IDLE, -1);
    priority = 0;
    flags &= ~0x10000000;
}

// 0x473970  Confidence: exact. The summon's emerge move (type 0xc).
void ai_state::begin_emerge()
{
    move_info *m;

    set_mode(AI_STATE_SUMMONING, -1);
    owner->process_anim(reported_frametime);
    owner->obj_flags |= 1;
    m = owner->find_move(0xc, 0);
    if (m)
        emerge_move = owner->play_move((int)m, 1.0f, 0, 0, 0);
    else
        emerge_move = -1;
}

// 0x4739e0  Confidence: exact.
void ai_state::class_die()
{
    if (owner->find_move(6, 0x22)) {
        printf("AI_CLASS : DIE\n");
        owner->kill(0x22);
    } else {
        printf("AI_CLASS : OF_SHOULD_BE_DEAD\n");
        fx_end_owned(owner->handle, false);
        owner->obj_flags |= 4;
    }
}

// 0x473a40  Confidence: likely (the three stores are scheduled y, x, z).
// The point is passed by value as one vector3 (D1713: living_entity.cpp builds it in the argument slot).
void ai_state::travel_to_xyz(vector3 dest)
{
    travel_dest.x = dest.x;
    travel_dest.y = dest.y;
    travel_dest.z = dest.z;
    priority = AI_DIRECT_PRIORITY;
    if (!is_traveling_sub1()) {
        set_mode(AI_STATE_TRAVELING, 1);
        goals_clear();
    }
    flags &= ~0x10000000;
}

// 0x473a90  Confidence: likely (void or the tail of set_mode's result: `call; ret 8`).
void ai_state::travel_to_point(vector3 *pos, int n)
{
    travel_dest = *pos;
    priority = n;
    set_mode(AI_STATE_TRAVELING, 2);
}

// ---- paths -----------------------------------------------------------------------------------------
// 0x473ac0  Confidence: likely (the squared distance's operand order). Drops a one-node path that ends
// within 5 cm of its start (not for traveling sub 8), restarts the step counter, and for a moving
// character (not a fighting monster) aims ai_move_dir along the first step.
void ai_state::path_begin()
{
    float dx, dz;

    if (path && !path->next) {
        dx = path_start.x - path->pos.x;
        dz = path_start.z - path->pos.z;
        if (dz * dz + dx * dx < 0.0025f && submode != 8) {
            ai_path_free(path);
            path = NULL;
        }
    }
    if (!path)
        return;
    path_step = 0;
    path_started = 1;
    if (!(owner->forward_speed > 1.0f))
        return;
    if (owner->type == CHAR_TYPE_MONSTER && is_mode(AI_STATE_COMBAT, -1))
        return;
    ai_move_dir = owner->pos - path->pos;
    ai_move_dir.normalize_safe();
    flags |= 0x10000000;
}

// 0x473bc0  Confidence: exact.
ai_path *ai_state::set_path(vector3 *start, ai_path *p)
{
    ai_path_free(path);
    path = p;
    path_start = *start;
    path_begin();
    if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(owner) && path)
        mp_send_new_path(owner);
    return path;
}

// 0x473c40  Confidence: likely (the mount walk). The search starts from the bottom of the mount chain,
// with its size and layer, and the path is stored on that character's AI. The cost bias gets the
// char_info +0x260 value added (D1904).
ai_path *ai_state::path_find_new(vector3 *from, int from_layer, vector3 *to, int to_layer, int size,
                                 float cost_bias)
{
    ai_state *ai = this;
    Character *m;

    for (m = owner->get_mount(); m; m = ai->owner->get_mount()) {
        size = m->info->size;
        from_layer = m->layer;
        ai = &m->ai;
        from = &m->pos;
    }
    ai_path_free(ai->path);
    ai->path = NULL;
    cost_bias += ai->owner->info->head_height;
    ai->path = ai_path_find(from, from_layer, to, to_layer, size, cost_bias);
    ai->path_start = *from;
    ai->path_timer_a.SetTimeout(1000);
    ai->path_timer_b.SetTimeout(1000);
    if (!character_uses_pathfinding(ai->owner))
        ai->path_flags |= 8;
    if (ai->is_mode(AI_STATE_COMBAT, -1))
        ai->path_flags |= 8;
    ai->path_begin();
    if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(ai->owner) && ai->path)
        mp_send_new_path(ai->owner);
    return ai->path;
}

// 0x473d80  Confidence: likely. Appends a search from the last node, or starts a new path (only then
// is path_start set). Returns the new piece.
ai_path *ai_state::path_find_append(vector3 *from, int from_layer, vector3 *to, int to_layer, int size,
                                    float cost_bias)
{
    ai_path *n = path;
    ai_path *piece;

    while (n && n->next)
        n = n->next;
    if (n) {
        piece = n->next = ai_path_find(&n->pos, n->layer, to, to_layer, size, cost_bias);
    } else {
        piece = path = ai_path_find(from, from_layer, to, to_layer, size, cost_bias);
        path_start = *from;
    }
    if ((game_flags & GF_MULTIPLAYER) && character_is_locally_controlled(owner) && path)
        mp_send_new_path(owner);
    return piece;
}

// 0x473e50  Confidence: likely. The search time counts towards the profiler total unless ai_do_frame
// is running (which counts it itself). The size is the sixth argument; the fifth is not read, and the
// cost bias is always 0.
int ai_state::path_to(vector3 *from, int from_layer, vector3 *to, int to_layer, int unused, int size,
                      bool append, bool keep_flag)
{
    int t0, t1;

    if (from_layer < 0) {
        ai_path_free(path);
        path = NULL;
        return -1;
    }
    t0 = timer_get(1000000);
    if (append)
        path_find_append(from, from_layer, to, to_layer, size, 0.0f);
    else
        path_find_new(from, from_layer, to, to_layer, size, 0.0f);
    t1 = timer_get(1000000);
    if (!ai_do_frame_active)
        ai_prof_path_us += t1 - t0;
    if (!path)
        return -1;
    if (!keep_flag)
        path_flags |= 8;
    else
        path_flags &= ~8;
    return 0;
}

// ---- goals -----------------------------------------------------------------------------------------
// 0x473f40  Confidence: exact. Running (not walking) sets goal flag 0x200.
int ai_state::add_travel(vector3 *pos, int layer, int priority, bool walk)
{
    unsigned int f = 0;

    if (!walk)
        f = 0x200;
    return goal_add(AI_GOAL_PATHFIND, priority, f, -1, pos, layer, AI_NO_SPELL, 0.0f);
}

// 0x473f70  Confidence: exact.
int ai_state::add_attack(Character *t, int priority)
{
    if (t->flags & CF_HIDDEN)
        return -1;
    if (!owner->is_enemy(t))
        return -1;
    if (flags & AI_FLAG_PLAYER_CONTROL)
        goals_remove_type1();
    return goal_add(AI_GOAL_ATTACK, priority, 0, t->handle, NULL, 0, AI_NO_SPELL, 0.0f);
}

// 0x473fd0  Confidence: likely (the flag-inverting conditions). Party members (owner flag 1) invert the
// request; a combat transition or a pending flagged goal inverts it again unless +0x50 bit 31.
int ai_state::set_combat_mode_goal(bool on, int priority)
{
    bool want;
    int pending, g;

    if (owner->flags & 1)
        want = !on;
    else
        want = on;
    pending = goal_find(3, AI_GOAL_SWITCH_COMBAT, 0, 8, -1, 0, 0, AI_NO_SPELL);
    if ((is_combat_e_or_sneak_attack() || pending != -1) && !(flags & 0x80000000))
        want = !want;
    if (!want) {
        g = goal_find(0x41, AI_GOAL_SWITCH_COMBAT, priority, 0, -1, 0, 0, AI_NO_SPELL);
        if (g >= 0 && goals[g].id != cur_goal && g != pending)
            goal_remove(g);
        return -1;
    }
    if (owner->type != CHAR_TYPE_PARTY && goal_find(1, AI_GOAL_SWITCH_COMBAT, 0, 0, -1, 0, 0, AI_NO_SPELL) != -1)
        return 0;
    return goal_alloc(AI_GOAL_SWITCH_COMBAT, priority, 0, -1, NULL, 0, AI_NO_SPELL);
}

// 0x4740c0  Confidence: exact.
int ai_state::add_cast_spell(int spell, int priority)
{
    return goal_add(AI_GOAL_CAST_SPELL, priority, 0, -1, NULL, 0, spell, 0.0f);
}

// 0x4740e0  Confidence: exact. Hidden targets (flag 8) only for spells 3 and 5.
int ai_state::add_cast_at_target(int spell, Character *t, int priority)
{
    if (!t)
        return -1;
    if ((t->flags & CF_HIDDEN) && spell != 5 && spell != 3)
        return -1;
    return goal_add(AI_GOAL_CAST_TARGETED, priority, 0, t->handle, NULL, 0, spell, 0.0f);
}

// 0x474130  Confidence: exact.
int ai_state::add_cast_at_point(int spell, vector3 *pos, int layer, int priority)
{
    return goal_add(AI_GOAL_CAST_LOCATION, priority, 0, -1, pos, layer, spell, 0.0f);
}

// 0x474160  Confidence: exact.
int ai_state::add_move_to(vector3 *pos, int layer, bool walk, int priority)
{
    unsigned int f = 0;

    if (walk == true)
        f = 0x10;
    return goal_add(AI_GOAL_GOTO_NAVPOINT, priority, f, -1, pos, layer, AI_NO_SPELL, 0.0f);
}

// 0x474190  Confidence: exact (D1903).
int ai_state::add_move_to_timed(vector3 *pos, int layer, float f, bool walk, int ms)
{
    unsigned int fl = 0;

    if (walk == true)
        fl = 0x10;
    return goal_add(AI_GOAL_WANDER, ms, fl, -1, pos, layer, AI_NO_SPELL, f);
}

// 0x4741d0  Confidence: likely (the dot product's operand order). The backstab goal when the owner stands
// within 2 m behind the target.
int ai_state::add_skill_from_behind(Character *t, int priority)
{
    vector3 d, back;

    if (!(owner->distance_between_edges(t, true, -1) <= 2.0f))
        return -1;
    d = owner->pos - t->pos;
    d.normalize();
    back = -t->orient.fvec;
    if (!(d.z * back.z + d.y * back.y + d.x * back.x > 0.707f))
        return -1;
    return goal_add(AI_GOAL_BACKSTAB, priority, 0, t->handle, NULL, 0, AI_NO_SPELL, 0.0f);
}

// ---- disabling ---------------------------------------------------------------------------------------
// 0x4742b0  Confidence: exact. A second knock-down while down only restarts the mode timer when it is
// longer than the time left, keeping the old duration (NOTES.md).
int ai_state::knock_down_local(int ms)
{
    if (!can_be_knocked_down())
        return 0;
    if (is_knocked_down() == true) {
        if (ms > disable_ms - timer.GetElapsedTime())
            timer.SetTimeout(0);
        return 1;
    }
    disable_ms = ms;
    disable_timer.invalidate();
    set_mode(AI_STATE_DISABLED, AIS_TRIPPED_FALL);
    if (owner == cur_character)
        hud_show_message(XSTR("Knocked Down!", 6));
    return 1;
}

// 0x474350  Confidence: exact.
void ai_state::knock_down(int ms)
{
    if (knock_down_local(ms) && (game_flags & GF_MULTIPLAYER))
        mp_broadcast_ai_disable(owner, 0, disable_ms);
}

// 0x474390  Confidence: exact. Pushed (sub 0x2f), unless the record cannot be pushed or rides.
int ai_state::disable_2f_local()
{
    if (owner->info->flags & CIF_NO_PUSH)
        return 0;
    if (owner->get_mount())
        return 0;
    disable_timer.invalidate();
    set_mode(AI_STATE_DISABLED, AIS_PUSHED);
    return 1;
}

// 0x4743e0  Confidence: exact.
void ai_state::disable_2f()
{
    if (disable_2f_local() && (game_flags & GF_MULTIPLAYER))
        mp_broadcast_ai_disable(owner, 1, 0);
}

// 0x474410  Confidence: exact. Stunned (sub 0x31).
void ai_state::disable_31()
{
    disable_timer.invalidate();
    owner->set_flag_2_4(-1);
    set_mode(AI_STATE_DISABLED, AIS_STUNNED);
}

// 0x474440  Confidence: exact.
int ai_state::add_hide()
{
    if (is_mode(AI_STATE_HIDING, -1)) {
        go_idle();
        return 0;
    }
    return goal_add(AI_GOAL_HIDE, AI_DIRECT_PRIORITY, 0, -1, NULL, 0, AI_NO_SPELL, 0.0f);
}

// 0x474480  Confidence: exact.
int ai_state::add_sneak()
{
    return goal_add(AI_GOAL_SNEAK, AI_DIRECT_PRIORITY, 0, -1, NULL, 0, AI_NO_SPELL, 0.0f);
}

// ---- direct orders -----------------------------------------------------------------------------------
// 0x4744a0  Confidence: exact. The combat record's set_target ignores its first argument; every caller
// passes the owner (D1908).
void ai_state::attack_target(Character *t)
{
    set_mode(AI_STATE_COMBAT, AIS_CLOSE_COMBAT);
    priority = AI_DIRECT_PRIORITY;
    owner->combat.set_target((int)owner, t);
    target = t->handle;
}

// 0x4744e0  Confidence: exact.
void ai_state::attack_target_sub10(Character *t)
{
    set_mode(AI_STATE_COMBAT, AIS_RANGED_COMBAT);
    priority = AI_DIRECT_PRIORITY;
    owner->combat.set_target((int)owner, t);
    target = t->handle;
}

// 0x474520  Confidence: exact. The multiplayer host starts a backstab (0x11), the others close combat.
void ai_state::net_attack_target(Character *t)
{
    if (!t)
        return;
    set_mode(AI_STATE_COMBAT, MP_IS_HOST() ? AIS_BACKSTAB : AIS_CLOSE_COMBAT);
    priority = AI_DIRECT_PRIORITY;
    owner->combat.set_target((int)owner, t);
    target = t->handle;
}

// 0x474580  Confidence: exact.
void ai_state::start_travel(bool walk)
{
    priority = AI_DIRECT_PRIORITY;
    set_mode(AI_STATE_TRAVELING, 0);
    path_step = 0;
    if (walk)
        owner->set_speed_scale(owner->walk_speed, false);
    else
        owner->set_speed_scale(owner->run_speed, false);
}

// 0x4745d0  Confidence: exact.
void ai_state::charge_target(Character *t)
{
    travel_dest = t->pos;
    target = t->handle;
    owner->combat.set_target((int)owner, t);
    priority = AI_DIRECT_PRIORITY;
    set_mode(AI_STATE_COMBAT, AIS_CHARGE);
}

// 0x474620  Confidence: exact.
void ai_state::travel_to_current_dest()
{
    priority = AI_DIRECT_PRIORITY;
    set_mode(AI_STATE_TRAVELING, 1);
    flags &= ~0x10000000;
    goals_clear();
}

// 0x474650  Confidence: exact.
void ai_state::enter_sneaking()
{
    disable_timer.invalidate();
    if (owner->flags & 0x400) {
        timer44.invalidate();
        set_mode(AI_STATE_SNEAKING, AIS_LEAVE_SNEAKING);
    } else {
        set_mode(AI_STATE_SNEAKING, AIS_ENTER_SNEAKING);
    }
}

// 0x474690  Confidence: exact.
void ai_state::enter_hiding(int enter)
{
    disable_timer.invalidate();
    if (enter)
        set_mode(AI_STATE_HIDING, AIS_ENTER_HIDING);
    else
        set_mode(AI_STATE_HIDING, AIS_LEAVE_HIDING);
}

// ---- savegame (the LENT record, character_save_state / _load_state) ---------------------------------
// 0x4746d0  Confidence: likely (the floats go through a stack temporary: fld / fstp / mov, so the
// copy is written through a float local). The combat style, and for NPCs and monsters the five goals
// (0x30 bytes each; the target as the name hash of the character, "" when none; the path not saved).
int ai_state::save(ubyte *out)
{
    int n = 4;
    int i;
    ai_goal *g;
    Character *t;
    float f;

    *(int *)out = combat_style;
    if (owner->type == CHAR_TYPE_MONSTER || owner->type == CHAR_TYPE_NPC) {
        for (i = 0; i < 5; i++) {
            g = &goals[i];
            *(int *)(out + n) = g->type;      n += 4;
            *(int *)(out + n) = g->flags;     n += 4;
            *(int *)(out + n) = g->id;        n += 4;
            f = g->time;  *(int *)(out + n) = *(int *)&f; n += 4;
            *(int *)(out + n) = g->priority;  n += 4;
            f = g->pos.x; *(int *)(out + n) = *(int *)&f; n += 4;
            f = g->pos.y; *(int *)(out + n) = *(int *)&f; n += 4;
            f = g->pos.z; *(int *)(out + n) = *(int *)&f; n += 4;
            *(int *)(out + n) = g->layer;     n += 4;
            *(int *)(out + n) = g->spell;     n += 4;
            t = living_from_handle(g->target);
            *(int *)(out + n) = name_hash_nocase(t ? t->get_name() : ""); n += 4;
            f = g->arg;   *(int *)(out + n) = *(int *)&f; n += 4;
        }
    }
    return n;
}

// 0x4747e0  Confidence: likely. Goal targets are found again by the name hash: the first living entity
// with that name (NOTES.md).
int ai_state::load(const ubyte *in)
{
    int n = 4;
    int i, hash;
    ai_goal *g;
    Character *c;

    combat_style = *(const int *)in;
    if (owner->type == CHAR_TYPE_MONSTER || owner->type == CHAR_TYPE_NPC) {
        for (i = 0; i < 5; i++) {
            g = &goals[i];
            g->type = *(const int *)(in + n);            n += 4;
            g->flags = *(const unsigned int *)(in + n);  n += 4;
            g->id = *(const int *)(in + n);              n += 4;
            g->time = *(const float *)(in + n);          n += 4;
            g->priority = *(const int *)(in + n);        n += 4;
            g->pos.x = *(const float *)(in + n);         n += 4;
            g->pos.y = *(const float *)(in + n);         n += 4;
            g->pos.z = *(const float *)(in + n);         n += 4;
            g->layer = *(const int *)(in + n);           n += 4;
            g->spell = *(const int *)(in + n);           n += 4;
            hash = *(const int *)(in + n);               n += 4;
            for (c = living_list.live_next; c != &living_list; c = c->live_next)
                if (name_hash_nocase(c->get_name()) == hash)
                    break;
            if (c != &living_list)
                g->target = c->handle;
            else
                g->target = -1;
            g->arg = *(const float *)(in + n);           n += 4;
        }
    }
    return n;
}

// ---- global ------------------------------------------------------------------------------------------
// Registered with atexit by ai_global_init; empty, folded into empty_function (0x4f13e0). Name ours.
static void ai_global_close()
{
}

// 0x474900  Confidence: likely (the cos / sin order and the zero store). 16 horizontal directions.
void ai_global_init()
{
    float angle = 0.0f;
    int i;

    pathfinding_init();
    for (i = 0; i < 16; i++) {
        ai_directions[i].x = (float)cos(angle);
        ai_directions[i].y = 0.0f;
        ai_directions[i].z = (float)sin(angle);
        angle += 0.3926991f;          // PI / 8
    }
    atexit(ai_global_close);
}

// 0x474950  Confidence: likely (the order of the nine averages follows the binary; jump tables 0x474cac
// and 0x474cc4 over the object types 4..9). Every frame every living entity gets ai_frame (type 4
// always, the others while not dead); there is no time budget (NOTES.md). The RealtimeTimer only paces
// the profiler averages. The pass's time is added to both the do_frame and the total counter.
void ai_do_frame()
{
    obj *o, *next;
    Character *c;
    int i, t0, dt;

    ai_do_frame_active = true;
    if (!ai_prof_timer.valid() || ai_prof_timer.elapsed()) {
        ai_prof.avg.think_d_us = ai_prof.acc.think_d_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.think_c_us = ai_prof.acc.think_c_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.frame_us = ai_prof.acc.frame_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.frame_b_us = ai_prof.acc.frame_b_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.frame_c_us = ai_prof.acc.frame_c_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.do_frame_us = ai_prof.acc.do_frame_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.think_b_us = ai_prof.acc.think_b_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.think_a_us = ai_prof.acc.think_a_us * 1000 / AI_PROF_PERIOD;
        ai_prof.avg.path_us = ai_prof.acc.path_us * 1000 / AI_PROF_PERIOD;
        ai_prof_timer.SetTimeout(AI_PROF_PERIOD);
        ai_prof.acc.think_d_us = 0;
        ai_prof.acc.think_c_us = 0;
        ai_prof.acc.frame_us = 0;
        ai_prof.acc.frame_b_us = 0;
        ai_prof.acc.frame_c_us = 0;
        ai_prof.acc.do_frame_us = 0;
        ai_prof.acc.think_b_us = 0;
        ai_prof.acc.think_a_us = 0;
        ai_prof.acc.path_us = 0;
    }
    t0 = timer_get(1000000);
    memset(ai_mode_frame_counts, 0, sizeof(ai_mode_frame_counts));
    for (i = 0; i < MAX_CAST_EFFECT_TRACKS; i++) {
        if (cast_effect_tracks[i].spell != -1) {
            if ((cast_effect_tracks[i].time -= reported_frametime) <= 0.0f)
                cast_effect_tracks[i].spell = -1;
        }
    }
    if (!cutscene_script_is_active())
        ai_groups_frame();
    for (o = obj_active_list.next; o != &obj_active_list; o = o->next) {
        switch (o->type) {
        case 4: case 5: case 7: case 9:
            ((Character *)o)->flags &= ~0x4000;
            break;
        }
    }
    for (o = obj_active_list.next; o != &obj_active_list; o = next) {
        next = o->next;
        switch (o->type) {
        case 4: case 5: case 7: case 9:
            c = (Character *)o;
            if (c->type == 4 || !(c->flags & CF_DEAD)) {
                c->last_pos = c->pos;
                c->last_orient = c->orient;
                ai_frame(c);
            }
            break;
        }
    }
    for (o = obj_active_list.next; o != &obj_active_list; o = o->next)
        if (o->type == CHAR_TYPE_BOSS)
            boss_ai_do_frame_hook((Character *)o);
    dt = timer_get(1000000) - t0;
    ai_prof_do_frame_us += dt;
    ai_prof_path_us += dt;
    ai_do_frame_active = false;
}

// 0x474ce0 init_ccmd_ai_state_thunk (jmp, static initialiser entry 226). Confidence: exact.
// 0x474cf0 init_ccmd_ai_state (console_command constructor). Confidence: exact.
console_command ccmd_ai_state("ai_state", "Change the AI state of the study object", cmd_ai_state);   // 0x22e48c0

// 0x474d10  Confidence: exact. The name test compares the table's pointers with next_arg's address
// (`cmp dword ptr [eax], 0x25dacec`), so no name ever matches (NOTES.md).
void cmd_ai_state()
{
    Character *c;
    int i;

    if (calling_a_command_function) {
        c = living_from_handle(world_mouse_target_handle);
        if (!c) {
            print_to_console("Error: No study object selected\n", 0);
            return;
        }
        process_command(ARG_STRING);
        for (i = 0; i < NUM_AI_STATES; i++)
            if (ai_state_names[i] == next_arg)
                break;
        if (i == NUM_AI_STATES) {
            print_to_console("Error: Invalid state name\n", 0);
            return;
        }
        c->ai.set_mode(i, -1);
    }
    if (doing_help_for_comand) {
        print_to_console("Usage: ai_state <state name>\n\n", 0);
        print_to_console("The following states are available:\n\n", 0);
        for (i = 0; i < NUM_AI_STATES; i++) {
            sprintf(string_buffer, "%s\n", ai_state_names[i]);
            print_to_console(string_buffer, 0);
        }
    }
}

// 0x474de0 init_ccmd_pause_ai_thunk (jmp, static initialiser entry 227). Confidence: exact.
// 0x474df0 init_ccmd_pause_ai (console_command constructor). Confidence: exact.
console_command ccmd_pause_ai("pause_ai", "Toggles Ai_paused", cmd_pause_ai);   // 0x22e48b0

// 0x474e10  Confidence: exact (the console's bool command macro).
void cmd_pause_ai()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Ai_paused = true;
        else if (arg_type & ARG_FALSE)
            Ai_paused = false;
        else if (arg_type & ARG_NONE)
            Ai_paused ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "pause_ai", "Ai_paused");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "pause_ai", Ai_paused ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x474ed0 init_ccmd_ai_wander_thunk (jmp, static initialiser entry 228). Confidence: exact.
// 0x474ee0 init_ccmd_ai_wander (console_command constructor). Confidence: exact.
console_command ccmd_ai_wander("ai_wander", "Toggles Ai_path_wandering_enabled", cmd_ai_wander);   // 0x22e48d0

// 0x474f00  Confidence: exact (the console's bool command macro).
void cmd_ai_wander()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Ai_path_wandering_enabled = true;
        else if (arg_type & ARG_FALSE)
            Ai_path_wandering_enabled = false;
        else if (arg_type & ARG_NONE)
            Ai_path_wandering_enabled ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "ai_wander", "Ai_path_wandering_enabled");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "ai_wander", Ai_path_wandering_enabled ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// ---- small mode helpers ------------------------------------------------------------------------------
// 0x474fc0  Confidence: exact.
void ai_state::enter_combat()
{
    set_mode(AI_STATE_COMBAT, -1);
}

// 0x474fd0  Confidence: exact.
bool ai_state::is_disabled_2c()
{
    return is_mode(AI_STATE_DISABLED, AIS_TRIPPED_FALL);
}

// 0x474fe0  Confidence: exact.
bool ai_state::is_knocked_down()
{
    return is_mode(AI_STATE_DISABLED, AIS_TRIPPED_DOWN);
}

// 0x474ff0  Confidence: exact.
bool ai_state::is_disabled_2e()
{
    return is_mode(AI_STATE_DISABLED, AIS_TRIPPED_UP);
}

// 0x475000  Confidence: likely (`mov eax, 1` / `xor eax, eax` for a bool that the callers test with
// `test al, al`).
bool ai_state::is_disabled_any()
{
    if (is_disabled_2c() || is_knocked_down() || is_disabled_2e())
        return true;
    return false;
}

// 0x475030  Confidence: exact.
bool ai_state::can_be_knocked_down()
{
    if (owner->info->flags & CIF_NO_TRIP)
        return false;
    if (is_combat_e_or_sneak_attack() == true)
        return false;
    if (owner->has_status(STATUS_PARALYZE) == true)
        return false;
    if (owner->get_mount())
        return false;
    if (is_knocked_down() == true)
        return false;
    if (is_disabled_2e() == true)
        return false;
    return true;
}

// ---- tracked cast effects (module "-" in sum.symbols; this unit's data, D1905) ----------------------
// 0x475090  Confidence: exact.
void cast_effect_tracks_reset()
{
    int i;

    for (i = 0; i < MAX_CAST_EFFECT_TRACKS; i++)
        cast_effect_tracks[i].spell = -1;
}

// 0x4750b0  Confidence: likely (effect_get_base is called again for the time, not kept). Remembers the
// effect of a spell below 0x26 for its animation's length * 1.5 (15 frames per second).
void cast_effect_track_add(int spell, int handle)
{
    effect_object *fx = (effect_object *)obj_from_handle_b(handle);
    int i;

    if (!fx || !fx->instance || !effect_get_base(fx->instance->id_bits >> 9))
        return;
    if (spell >= 0x26)
        return;
    for (i = 0; i < MAX_CAST_EFFECT_TRACKS; i++)
        if (cast_effect_tracks[i].spell == -1)
            break;
    if (i == MAX_CAST_EFFECT_TRACKS)
        return;
    cast_effect_tracks[i].spell = spell;
    cast_effect_tracks[i].handle = handle;
    cast_effect_tracks[i].time = effect_get_base(fx->instance->id_bits >> 9)->num_frames * (1.0f / 15.0f) * 1.5f;
}

// 0x475160  Confidence: exact.
int cast_effect_track_count()
{
    int n = 0;
    int i;

    for (i = 0; i < MAX_CAST_EFFECT_TRACKS; i++)
        if (cast_effect_tracks[i].spell != -1)
            n++;
    return n;
}
