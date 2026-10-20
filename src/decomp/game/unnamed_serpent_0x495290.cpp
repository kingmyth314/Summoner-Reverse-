// unnamed_serpent_0x495290.cpp - Sum.exe v1.40, code 0x495290-0x497aba: the Serpent Rider boss (Liangshan,
// Jadetemple and the random forest level): stage changes, the mount, the attack choice of the serpent
// and its two illusion copies ("parts"), the illusion, the whip, the console commands serpent_stage /
// serpent_unlink / serpent_solo / serpent_combat.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: init_serpent_timers (entry 273, 0x495290) after the snow unit's cmd_snow_render, up to
// serpent_set_solo (0x497a40-0x497aba); the phoenix unit's first console initialiser (entry 278)
// follows at 0x497ac0.
// Data: .bss 0x2451d80-0x2452417 (the move ids, the fight state object 0x2451d98-0x24523d7 (D3403), the
// four console objects with two more move ids between them, serpent_mount), .data 0x59c65c-0x59c80b
// (serpent_boss_needs_setup, the console and animation strings, "serp_boss.tbl", "$sit",
// "Serp-illusion.vfx"). "Liangshan", "Jadetemple", "ready", "walk", ... and the float constants are
// pooled; 0.85 (0x581b14), 6.25 and 49.0 (0x581b18 / 0x581b1c) are this unit's .rdata.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/ai/unnamed_serpent_0x495290.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/effects/fx.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"

#define CHAR_TYPE_BOSS 9
#define CF_UNTARGETABLE 0x20000       // Character +0x320: skipped by the AI groups' enemy scan (name ours)

// ---- .bss / .data ----------------------------------------------------------------------------------
int serpent_attack_anim_a;                                    // 0x2451d80
int serpent_attack_anim_b;                                    // 0x2451d84
int serpent_attack_anim_c;                                    // 0x2451d88
int serpent_illusion_anim;                                    // 0x2451d8c
int serpent_attack_anim_d;                                    // 0x2451d90
// 0x495290 init_serpent_timers_thunk (jmp, static initialiser entry 273). Confidence: exact.
// 0x4952a0 init_serpent_timers: the compiler-generated constructor of serpent_boss (its 13 GameTimers in
//          member order: four, the illusion timer, the five threat timers, three). Confidence: exact.
serpent_boss serpent;                                         // 0x2451d98
int serpent_attack_anim_e;                                    // 0x24523e4
int serpent_attack_anim_f;                                    // 0x24523f4
Character *serpent_mount;                                     // 0x2452414
bool serpent_boss_needs_setup = true;                         // 0x59c65c

// ---- console commands -------------------------------------------------------------------------------
// 0x495310 init_ccmd_serpent_stage_thunk (jmp, static initialiser entry 274). Confidence: exact.
// 0x495320 init_ccmd_serpent_stage (console_command constructor). Confidence: exact.
console_command ccmd_serpent_stage("serpent_stage", "Set Serpent rider's current combat stage",
                                   cmd_serpent_stage);       // 0x24523e8

// 0x495340  Confidence: exact. No calling_a_command_function test, no help or status text.
void cmd_serpent_stage()
{
    process_command(ARG_INT);
    if ((arg_type & ARG_INT) && command_argument_int >= 0 && command_argument_int <= 3)
        serpent_target_stage = command_argument_int;
}

// 0x495370 init_ccmd_serpent_unlink_thunk (jmp, static initialiser entry 275). Confidence: exact.
// 0x495380 init_ccmd_serpent_unlink (console_command constructor). Confidence: exact.
console_command ccmd_serpent_unlink("serpent_unlink", "Unlinks serpent to mount", cmd_serpent_unlink);   // 0x24523f8

// 0x4953a0  Confidence: exact (VC6 byte match) (the store order of the two unlinks). rider_unlink_mount written out.
void cmd_serpent_unlink()
{
    Character *c = living_from_handle(serpent_handle);
    if (c == NULL)
        return;
    Character *m = c->get_mount();
    if (m == NULL)
        return;
    m->mount_flags = 0;
    m->mount_link = -1;
    m->ai.target = -1;
    m->flags &= ~0x20;
    c->mount_flags = 0;
    c->mount_link = -1;
    c->flags |= CF_COMBAT_READY;
}

// 0x495410  Confidence: exact. cmd_serpent_solo, ls_rand_forest01_frame.
Character *serpent_get_rider()
{
    return living_from_handle(serpent_handle);
}

// 0x495420 init_ccmd_serpent_solo_thunk (jmp, static initialiser entry 276). Confidence: exact.
// 0x495430 init_ccmd_serpent_solo (console_command constructor). Confidence: exact.
console_command ccmd_serpent_solo("serpent_solo", "Set serpent without horse\n", cmd_serpent_solo);   // 0x24523d8

// 0x495450  Confidence: exact.
void cmd_serpent_solo()
{
    Character *c = serpent_get_rider();
    if (c)
        serpent_set_solo(c);
}

// 0x495470 init_ccmd_serpent_combat_thunk (jmp, static initialiser entry 277). Confidence: exact.
// 0x495480 init_ccmd_serpent_combat (console_command constructor). Confidence: exact.
console_command ccmd_serpent_combat("serpent_combat", "Sets serpent rider in combat state",
                                    cmd_serpent_combat);     // 0x2452408

// 0x4954a0  Confidence: exact.
void cmd_serpent_combat()
{
    serpent_combat = true;
    print_to_console("Setting serpent's ai state to combat\n", 0);
}

// ---- setup -----------------------------------------------------------------------------------------
// 0x4954c0  Confidence: exact (VC6 byte match) (the buffer sizes). The flinch moves "stageN_flinch_area" /
// "stageN_flinch_status" of the serpent's flinch list; stage 3 uses stage 2's. The message for a bad
// name is formatted into a local buffer and never shown (NOTES.md).
void serpent_rider_set_flinch(move_info *moves)
{
    char num[2];
    char msg[0x80];

    for (move_info *m = moves; m; m = m->next) {
        const char *p = m->action;
        if (p == NULL || strnicmp(p, "stage", strlen("stage")) != 0)
            continue;
        p += strlen("stage");
        num[0] = *p;
        num[1] = 0;
        int stage = atoi(num) - 1;
        p += strlen("_flinch_") + 1;
        if (stricmp(p, "area") == 0)
            serpent.flinch_area[stage] = m;
        else if (stricmp(p, "status") == 0)
            serpent.flinch_status[stage] = m;
        else
            sprintf(msg, "Unable to set Serpent Rider flinch for %s -- bad $Action\n", m->anim_file);
    }
    serpent.flinch_area[2] = serpent.flinch_area[1];
    serpent.flinch_status[2] = serpent.flinch_status[1];
}

// 0x4955e0  Confidence: exact (VC6 byte match, match31; was likely; one vector3 local assigned twice, `front = c->pos + fvec` for the binary's fvec-first sum, D9001). Mounted: the ride
// animations, the Fangsickle, the timers reset, and the two illusion parts in front of and behind it
// (species 0x59, inactive and hidden until the illusion).
void serpent_spawn_rider(Character *c)
{
    int anim;

    boss_ensure_mount(c);
    c->anim_tripped_liedown = -1;
    anim = charinfo_find_anim(c->info, "ride_horse_ready");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "ride_horse_walk");
    if (anim != -1) {
        c->anim_norm_walk = anim;
        c->anim_walk = anim;
        c->anim_norm_walk_hurt = anim;
        c->anim_walk_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "ride_horse_run");
    if (anim != -1) {
        c->anim_norm_run = anim;
        c->anim_run = anim;
        c->anim_norm_run_hurt = anim;
        c->anim_run_hurt = anim;
    }
    c->attachments[0].item_type = item_type_find("Fangsickle");
    c->attachments[0].slot = 10;
    c->num_attachments = 1;
    serpent_timer_attack.SetTimeout(0);
    serpent_place_timer.SetTimeout(0);
    serpent_illusion_active_timer.invalidate();
    serpent_timer_a.invalidate();
    serpent.illusion_fx_on = false;
    serpent_first_attack = true;
    serpent_num_extra_parts = 0;

    int info = charinfo_index_from_species(SPECIES_SERPENT_RIDER);
    vector3 front;
    front = c->pos + c->orient.fvec;
    Character *p = living_entity_create(CHAR_TYPE_BOSS, info, &front, &c->orient, true, c->layer, 0, 0, false);
    if (p) {
        serpent_part_handles[serpent_num_extra_parts] = p->handle;
        serpent_num_extra_parts++;
        p->set_ai_active(0);
        p->flags |= CF_HIDDEN;
    }
    front = c->pos - c->orient.fvec;
    p = living_entity_create(CHAR_TYPE_BOSS, info, &front, &c->orient, true, c->layer, 0, 0, false);
    if (p) {
        serpent_part_handles[serpent_num_extra_parts] = p->handle;
        serpent_num_extra_parts++;
        p->set_ai_active(0);
        p->flags |= CF_HIDDEN;
    }
}

// 0x495830  Confidence: exact. On foot (stage 2): the walking animations, then off the mount.
void serpent_setup_anims(Character *c)
{
    int anim;

    anim = charinfo_find_anim(c->info, "ready");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
    }
    anim = charinfo_find_anim(c->info, "ready_hurt");
    if (anim != -1) {
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "walk");
    if (anim != -1) {
        c->anim_norm_walk = anim;
        c->anim_walk = anim;
        c->anim_norm_walk_hurt = anim;
        c->anim_walk_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "run");
    if (anim != -1) {
        c->anim_norm_run = anim;
        c->anim_run = anim;
        c->anim_norm_run_hurt = anim;
        c->anim_run_hurt = anim;
    }
    c->anim_tripped_liedown = charinfo_find_anim(c->info, "tripped_liedown");
    boss_release_mount(c);
}

// 0x495910  Confidence: exact.
void serpent_deactivate_parts(Character *c)
{
    serpent_setup_anims(c);
    for (int i = 1; i < serpent_num_extra_parts + 1; i++) {
        Character *p = living_from_handle(serpent.handles[i]);
        if (p)
            boss_part_deactivate(p);
    }
}

// 0x495960  Confidence: exact (VC6 byte match, match31; was likely; the has_rider test written as one if / else; the handle stored before the part count, D9001). The first load of a
// level clears the whole state; Liangshan and Jadetemple start on foot in stage 3. has_rider is set
// before the clear, so the clear always leaves it false (NOTES.md).
void ai_state::serpent_boss_load()
{
    serpent_dismount_stage_reached = false;
    serpent_stage_trigger = false;
    if (stricmp(level_filename, "Liangshan") == 0 || stricmp(level_filename, "Jadetemple") == 0)
        serpent_has_rider = false;
    else
        serpent_has_rider = true;
    if (serpent_boss_needs_setup != true)
        return;

    serpent_boss_needs_setup = false;
    memset((void *)&serpent, 0, sizeof(serpent));
    serpent_handle = owner->handle;
    serpent_num_extra_parts = 0;
    if (stricmp(level_filename, "Liangshan") == 0 || stricmp(level_filename, "Jadetemple") == 0) {
        serpent_stage = 3;
        serpent_target_stage = 3;
    } else {
        serpent_stage = 1;
        serpent_target_stage = 0;
    }
    serpent_attack_anim_a = 1;
    serpent_attack_anim_e = 2;
    serpent_attack_anim_b = 3;
    serpent_attack_anim_f = 4;
    serpent_attack_anim_d = 5;
    serpent_attack_anim_c = 6;
    serpent_illusion_anim = 7;
    if (stricmp(level_filename, "Liangshan") == 0 || stricmp(level_filename, "Jadetemple") == 0)
        serpent_deactivate_parts(owner);
    else
        serpent_spawn_rider(owner);
    for (int i = 0; i < BOSS_MAX_THREATS; i++)
        serpent_threat_table[i].handle = -1;

    serpent_num_offense_points = 0;
    for (int k = 0; k < 10; k++) {
        int tag = owner->instance->character->find_mesh_tag(offense_point_name(k));
        if (tag != -1) {
            serpent.offense_points[serpent_num_offense_points].tag = tag;
            serpent_num_offense_points++;
        }
    }
    boss_ai_parse_table("serp_boss.tbl", SERPENT_STAGES, serpent_attack_table, serpent_num_offense_points);
    serpent_rider_set_flinch((move_info *)owner->info->moves[4]);
    if (stricmp(level_filename, "Liangshan") != 0 && stricmp(level_filename, "Jadetemple") != 0)
        serpent_seat_rider();
    serpent_mount = owner->get_mount();
}

// 0x495b90  Confidence: exact (VC6 byte match) (the case order of the forced stage). The serpent itself only: a stage
// forced by serpent_stage / the escape cutscene, else by HP (below 25 % stage 3, below 50 % stage 2
// once the level script answered the dismount request).
void ai_state::serpent_check_stage()
{
    if (serpent_part_index(owner) != 0)
        return;
    if (serpent_target_stage != 0) {
        if (serpent_stage == serpent_target_stage)
            return;
        switch (serpent_target_stage) {
        default:
            serpent_target_stage = 0;
            return;
        case 3:
            serpent_deactivate_parts(owner);
            serpent_stage = 3;
            return;
        case 2:
            serpent_setup_anims(owner);
            serpent_stage = 2;
            return;
        case 1:
            serpent_spawn_rider(owner);
            serpent_stage = 1;
            return;
        }
    }
    float frac = owner->hp / (float)owner->get_max_stat(0);
    if (frac < 0.25f) {
        if (serpent_stage != 3) {
            serpent_deactivate_parts(owner);
            serpent_stage = 3;
        }
        return;
    }
    if (frac < 0.5f) {
        if (serpent_stage == 2)
            return;
        if (!serpent_dismount_stage_reached) {
            serpent_dismount_stage_reached = true;
            serpent_stage_trigger = false;
            return;
        }
        if (serpent_stage_trigger) {
            serpent_setup_anims(owner);
            serpent_stage = 2;
        }
        return;
    }
    if (serpent_stage != 1) {
        serpent_spawn_rider(owner);
        serpent_stage = 1;
    }
}

// 0x495cf0  Confidence: exact. ai_state_shutdown (boss_ai_unload).
void serpent_unload()
{
    serpent_boss_needs_setup = true;
}

// 0x495d00  Confidence: likely (the blend record through loco_blend_set). character_boss_anim_remap_in:
// the rider's stand / walk / run entries are replaced by the mount's (stand 3 = ready when the rider is
// in combat stance); remap_out copies the rider's own loco anims back into the remapped slots (D6400).
void serpent_anim_remap_in(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    Character *mount = c->get_mount();
    if (mount == NULL)
        return;
    int ready = 0;
    Character *r = mount->get_rider();
    if (r)
        ready = r->flags & CF_COMBAT_READY;
    serpent_anim_remap_count = set->count;
    for (int i = 0; i < set->count; i++) {
        int a = set->anim[i];
        if (a == mount->anim_stand) {
            if (ready)
                serpent.remap_slot[i] = 3;
            else
                serpent.remap_slot[i] = 0;
        } else if (a == mount->anim_walk) {
            serpent.remap_slot[i] = 4;
        } else if (a == mount->anim_run) {
            serpent.remap_slot[i] = 5;
        } else {
            continue;
        }
        serpent.remap_weight[i] = set->weight[i];
    }
}

// 0x495db0  Confidence: exact (VC6 byte match). LOGIC FIX (D6400): the slots get the character's own loco anims (EBX,
// the argument); the mount is only tested for NULL. The source copied mount->loco_anims.
void serpent_anim_remap_out(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    Character *mount = c->get_mount();
    if (mount == NULL)
        return;
    for (int i = 0; i < serpent_anim_remap_count; i++) {
        set->anim[i] = c->loco_anims[serpent.remap_slot[i]];
        set->weight[i] = serpent.remap_weight[i];
    }
}

// 0x495e10  Confidence: exact (the matrix copy is matrix::operator= expanded; the success path nested, the
// shared `return false` last, D7700). Every frame: the mount's random sound, then the rider at the
// mount's "$sit" tag (or at the mount's origin); the mount gets the rider's target and the rider's
// animation time.
bool ai_state::serpent_seat_rider()
{
    if (serpent_mount)
        boss_rider_random_sound(&serpent_mount->pos);
    if (serpent_has_rider) {
        Character *m = owner->get_mount();
        if (m != NULL) {
            m->ai.target = target;
            float t;
            m->instance->get_set_time(&t, false);
            owner->instance->get_set_time(&t, true);
            int tag = m->instance->character->find_mesh_tag("$sit");
            if (tag != -1) {
                vector3 pos;
                matrix orient;
                m->instance->get_bone_world_transform(tag, &m->orient, &m->pos, &orient, &pos);
                owner->pos = pos;
                owner->orient = orient;
                return true;
            }
            owner->pos = m->pos;
            owner->orient = m->orient;
            return true;
        }
    }
    return false;
}

// ---- attack choice ---------------------------------------------------------------------------------
// 0x495fa0  Confidence: exact. ai_state +0x50 bit 0x10 off, the part's attack done.
void ai_state::serpent_ai_idle()
{
    flags &= ~0x10;
    serpent_part_busy[serpent_part_index(owner)] = NULL;
}

// 0x495fc0  Confidence: exact (case order from the bodies' order: 3, 0, 1/2, default).
float serpent_target_weight(int species)
{
    switch (species) {
    case 3:
        return 0.7f;
    case 0:
        return 0.3f;
    case 1:
    case 2:
        return 0.05f;
    default:
        return 0.02f;
    }
}

// 0x496000  Confidence: exact. A weight per candidate by hero (a NULL candidate leaves its weight unset).
int serpent_pick_target(boss_target *list, int count)
{
    float weights[5];
    for (int i = 0; i < count; i++) {
        if (list[i].character)
            weights[i] = serpent_target_weight(list[i].character->info->species);
    }
    return rand_weighted_index(weights, count);
}

// 0x496060  Confidence: exact. Asleep for more than 10 s and not poisoned.
bool serpent_part_is_asleep(boss_target *list, int i)
{
    if (i == -1)
        return false;
    boss_target *t = &list[i];
    if ((t->flags & SERPENT_TF_ASLEEP) && !(t->flags & SERPENT_TF_POISONED) && t->character
        && t->character->status_time_left(STATUS_SLEEP) > 10000)
        return true;
    return false;
}

// 0x4960a0  Confidence: exact. Paralysed, asleep (the caller's flag) or in AI mode (8, 0x2d).
bool serpent_part_is_busy(boss_target *list, bool asleep, int i)
{
    if (i != -1) {
        boss_target *t = &list[i];
        Character *c = t->character;
        if ((t->flags & SERPENT_TF_PARALYZED) || asleep)
            return true;
        if (c != NULL && c->ai.is_mode(AI_STATE_DISABLED, 0x2d))
            return true;
    }
    return false;
}

// 0x4960f0  Confidence: likely (serpent_part_is_busy's body repeated rather than called).
bool serpent_part_is_busy_or_asleep(boss_target *list, int i)
{
    bool asleep = serpent_part_is_asleep(list, i);
    if (i != -1) {
        boss_target *t = &list[i];
        Character *c = t->character;
        if ((t->flags & SERPENT_TF_PARALYZED) || asleep)
            return true;
        if (c != NULL && c->ai.is_mode(AI_STATE_DISABLED, 0x2d))
            return true;
    }
    return false;
}

// 0x496150  Confidence: exact.
void serpent_timer_a_set_random(int lo, int hi)
{
    serpent_timer_a.SetTimeout((int)(frand() * (hi - lo)) + lo);
}

// 0x496180  Confidence: exact.
void serpent_timer_b_set_random(int lo, int hi)
{
    serpent_timer_b.SetTimeout((int)(frand() * (hi - lo)) + lo);
}

// 0x4961b0  Confidence: exact (VC6 byte match, match31; was likely; dir = target - boss with y cleared, the facing tests through the inline vector3::dot, D9001). The
// statuses, and in stage 3 the facing and whip tests (the whip is attacks[2][0]).
void serpent_evaluate_target(Character *boss, boss_target *t)
{
    Character *c = t->character;
    if (c == NULL)
        return;
    if (c->status_time_left(STATUS_SLEEP) > 0)
        t->flags |= SERPENT_TF_ASLEEP;
    if (t->character->status_time_left(STATUS_PARALYZE) > 0)
        t->flags |= SERPENT_TF_PARALYZED;
    if (t->character->status_time_left(STATUS_POISON) > 0)
        t->flags |= SERPENT_TF_POISONED;
    if (t->character->status_time_left(STATUS_SILENCE) > 0)
        t->flags |= SERPENT_TF_SILENCED;
    if (t->character->status_time_left(STATUS_BLIND) > 0)
        t->flags |= SERPENT_TF_BLIND;
    if (serpent_stage != 3)
        return;

    if (t->character->ai.is_mode(AI_STATE_DISABLED, 0x2d))
        t->flags |= SERPENT_TF_DISABLED;
    vector3 dir = t->character->pos - boss->pos;
    dir.y = 0;
    dir.normalize_safe();
    if (-(dir.dot(t->character->orient.fvec)) > 0.707f
        && dir.dot(boss->orient.fvec) > 0.0f)
        t->flags |= SERPENT_TF_BEHIND;
    if (boss->distance_between_edges(t->character, true, -1) < serpent_whip_reach
        && dir.dot(boss->orient.fvec)
               > serpent_attack_table[2][0].cos_max_angle)
        t->flags |= SERPENT_TF_WHIP;
}

// 0x496330  Confidence: unsure (the flag variables of the stage 1 branch; see NOTES.md). Returns 0 out
// of combat, 5 while the part is busy, 1 for a wander (pending sub-mode 0x20), else 3 after choosing
// the move of this part (serpent_part_busy / _move / _target) and ai_set_mode(3, 0x16).
// GAME BUG (NOTES.md): after a new target is picked in stage 1, serpent_part_is_asleep gets the old
// asleep flag (0 or 1) as the candidate index instead of the new target's index.
int ai_state::serpent_choose_attack()
{
    boss_target list[5];
    matrix tag_orient;
    vector3 tag_pos;
    float weights[4];
    int i, n, cur, h;
    bool asleep, busy;

    if (!serpent_combat)
        return 0;
    int idx = serpent_part_index(owner);
    if (owner->combat.is_busy() == true && serpent_part_busy[idx])
        return 5;
    if (owner->ai.is_mode(AI_STATE_COMBAT, 0x38) || owner->ai.is_mode(AI_STATE_COMBAT, 0x39) || owner->ai.is_mode(AI_STATE_COMBAT, 0x20))
        return 5;
    boss_attack **busy_slot = &serpent_part_busy[idx];
    if (*busy_slot)
        return 5;

    for (i = 0; i < serpent_num_offense_points; i++) {
        owner->instance->get_bone_world_transform(serpent.offense_points[i].tag, &owner->orient, &owner->pos,
                                                  &tag_orient, &tag_pos);
        serpent.offense_points[i].up = tag_orient.uvec;
        serpent.offense_points[i].pos = tag_pos + owner->centre_offset;
    }
    n = boss_target_list_build(owner, list, serpent_threat_table);
    cur = boss_target_list_find_b(list, n, target);
    if (cur == -1)
        return 0;
    owner->combat.set_special_skill_species89(0x55);
    if (!serpent_timer_a.valid()) {
        serpent_timer_a_set_random(20000, 30000);
        serpent_timer_b_set_random(5000, 10000);
    }
    bool a_elapsed = serpent_timer_a.elapsed();
    Character *tc = living_from_handle(target);
    if (tc && (tc->flags & CF_DEAD))
        tc = NULL;

    if (serpent_stage == 1) {
        asleep = serpent_part_is_asleep(list, cur);
        busy = serpent_part_is_busy(list, asleep, cur);
        bool special = false;
        if (serpent_timer_attack.elapsed())
            special = true;
        bool first = false;
        if (serpent_first_attack == true) {
            first = true;
            serpent_first_attack = false;
        }
        if ((special || first) && (!busy || frand() >= 0.3f || first)) {
            i = serpent_pick_target(list, n);
            h = target;
            if (i != -1)
                h = list[i].character->handle;
            serpent_timer_a_set_random(10000, 15000);
            serpent_part_target[idx] = h;
            serpent_part_move[idx] = 0xbc;
            *busy_slot = &serpent_attack_table[0][2];
        } else {
            if (serpent_timer_b.elapsed()) {
                if (count_attackers() > 1) {
                    serpent.pending_submode = 0x20;
                    return 1;
                }
                serpent_timer_b_set_random(5000, 10000);
            }
            h = target;
            if ((a_elapsed && (!busy || frand() >= 0.2f)) || tc == NULL) {
                int p = boss_target_list_pick(list, n, serpent_threat_table, target, false, 0);
                if (p != -1) {
                    h = p;
                    serpent_timer_a_set_random(20000, 30000);
                    cur = boss_target_list_find_b(list, n, p);
                    asleep = serpent_part_is_asleep(list, asleep);
                    busy = serpent_part_is_busy(list, asleep, cur);
                }
            }
            if (serpent_part_attack_count[idx] < 2 && frand() > 0.5f && !busy) {
                serpent_part_target[idx] = h;
                serpent_part_attack_count[idx]++;
                serpent_part_move[idx] = 0xba;
                *busy_slot = &serpent_attack_table[0][0];
            } else {
                if (asleep && n > 1 && frand() < 0.9f) {
                    int p = boss_target_list_pick(list, n, serpent_threat_table, target, true, 0);
                    if (p != -1) {
                        h = p;
                        serpent_timer_a_set_random(20000, 30000);
                    }
                }
                serpent_part_attack_count[idx] = 0;
                serpent_part_target[idx] = h;
                serpent_part_move[idx] = 0xbb;
                *busy_slot = &serpent_attack_table[0][1];
            }
        }
    } else if (serpent_stage == 2) {
        h = target;
        if (idx == 0 && a_elapsed) {
            if (!serpent_part_is_busy_or_asleep(list, cur) || frand() >= 0.85f) {
                int p = boss_target_list_pick(list, n, serpent_threat_table, h, false, 0);
                if (p != -1) {
                    h = p;
                    serpent_timer_a_set_random(20000, 30000);
                }
            }
        }
        cur = boss_target_list_find_b(list, n, h);
        serpent_part_target[idx] = target;
        if (serpent_part_attack_count[idx] < 2 && frand() > 0.5f && serpent_part_is_busy_or_asleep(list, cur)) {
            serpent_part_target[idx] = h;
            serpent_part_attack_count[idx]++;
            serpent_part_move[idx] = serpent_attack_anim_a;
            *busy_slot = &serpent_attack_table[1][1];
            owner->combat.set_special_skill_species89(0x50);
        } else {
            serpent_part_attack_count[idx] = 0;
            serpent_part_target[idx] = h;
            if (frand() < 0.2f) {
                serpent_part_move[idx] = serpent_attack_anim_e;
                *busy_slot = &serpent_attack_table[1][0];
                owner->combat.set_special_skill_species89(0x50);
            } else {
                serpent_part_move[idx] = serpent_attack_anim_b;
                *busy_slot = &serpent_attack_table[1][2];
            }
        }
    } else {
        h = target;
        if (a_elapsed) {
            if (!(list[cur].flags & SERPENT_TF_PARALYZED) || frand() >= 0.2f) {
                int p = boss_target_list_pick(list, n, serpent_threat_table, target, false, 0);
                if (p != -1) {
                    h = p;
                    serpent_timer_a_set_random(20000, 30000);
                }
            }
        }
        cur = boss_target_list_find_b(list, n, h);
        int in_whip = 0;
        for (i = 0; i < n; i++)
            if (list[i].flags & SERPENT_TF_WHIP)
                in_whip++;
        bool can_rear = false, can_front = false;
        unsigned int f = list[cur].flags;
        if ((f & SERPENT_TF_BEHIND) && !(f & SERPENT_TF_DISABLED))
            can_rear = true;
        if (!(f & SERPENT_TF_DISABLED))
            can_front = true;
        weights[0] = 5.0f;
        serpent_part_target[idx] = h;
        weights[1] = can_rear ? 1.0f : 0.0f;
        weights[2] = in_whip + 1.0f;
        weights[3] = can_front ? 2.0f : 0.0f;
        int k = rand_weighted_index(weights, 4);
        if (k == -1)
            k = rand() % 4;
        switch (k) {
        case 0:
            serpent_part_move[idx] = serpent_attack_anim_b;
            *busy_slot = &serpent_attack_table[2][3];
            break;
        case 1:
            serpent_part_move[idx] = serpent_attack_anim_d;
            *busy_slot = &serpent_attack_table[2][2];
            owner->combat.set_special_skill_species89(0x52);
            break;
        case 2:
            serpent_part_move[idx] = serpent_attack_anim_f;
            *busy_slot = &serpent_attack_table[2][0];
            owner->combat.set_special_skill_species89(0x53);
            break;
        case 3:
            serpent_part_move[idx] = serpent_attack_anim_c;
            *busy_slot = &serpent_attack_table[2][1];
            owner->combat.set_special_skill_species89(0x51);
            break;
        }
    }
    set_mode(AI_STATE_COMBAT, 0x16);
    return 3;
}

// 0x496b00  Confidence: exact. Sub-mode 0x20.
void ai_state::serpent_wander_update()
{
    if (!follow_path(0) || serpent_wander_timer.elapsed()) {
        serpent_timer_b_set_random(5000, 10000);
        set_mode(AI_STATE_COMBAT, 0x16);
    }
}

// ---- the illusion -----------------------------------------------------------------------------------
// 0x496b40  Confidence: exact. The party drops its attack goals and thinks again at once.
void serpent_illusion_freeze_party()
{
    ai_group *g = cur_character->ai.group;
    if (g == NULL)
        return;
    for (int i = 0; i < g->count; i++) {
        Character *m = living_from_handle(g->members[i]);
        if (m) {
            m->ai.goals_remove_type1();
            m->ai.go_idle();
        }
    }
    g->timer.SetTimeout(0);
}

// 0x496ba0  Confidence: likely. Sub-mode 0x38 (serpent only; a part that gets here is deactivated):
// the illusion move with Serp-illusion.vfx, then the serpent fades out over the last second and the
// copies take over (sub-mode 0x39).
// The loop that deactivates the old parts stops one short (i < serpent_num_extra_parts, the other
// loops use num + 1), so the second part is left as it is (NOTES.md).
void ai_state::serpent_illusion_start()
{
    int i;

    if (owner->handle != serpent_handle) {
        boss_part_deactivate(owner);
        return;
    }
    if (!serpent_illusion_active_timer.valid()) {
        for (i = 1; i < serpent_num_extra_parts; i++) {
            Character *p = living_from_handle(serpent.handles[i]);
            if (p)
                boss_part_deactivate(p);
        }
        move_info *m = owner->find_move(0, serpent_illusion_anim);
        if (m && owner->play_move((int)m, 1.0f, true, true, false) == -1) {
            go_idle();
            return;
        }
        fx_attach_to_character("Serp-illusion.vfx", owner->handle, -1, -1, 0, 0);
        serpent_illusion_active_timer.SetTimeout(1000 - (int)(owner->instance->get_anim_length(0) * -1000.0f));
        return;
    }
    int rem = serpent_illusion_active_timer.GetRemainingTime();
    if (rem > 2000)
        return;
    if (rem > 0) {
        int a = (rem - 1000) * 255 / 1000;
        if (a < 1)
            a = 1;
        owner->instance->_unkb4 = (ubyte)a;
        return;
    }
    owner->instance->_unkb4 = 0;
    serpent_illusion_active_timer.invalidate();
    serpent.illusion_fx_on = false;
    for (i = 0; i < serpent_num_extra_parts + 1; i++) {
        Character *p = living_from_handle(serpent.handles[i]);
        if (p)
            p->flags |= CF_UNTARGETABLE;
    }
    serpent_illusion_freeze_party();
    set_mode(AI_STATE_COMBAT, 0x39);
}

// 0x496d60  Confidence: likely (m-1020, D11104: the alpha bases as `a = 255; if (i) a = 230`; left: 8966 turns
// x / -1000 into base - x / 1000 and threads the rem <= 300 test; the alpha arithmetic divides by -1000 as the binary does). Sub-mode 0x39:
// all three at serpent_place_parts' points, invisible for 1.4 s, fading in with the effect, then
// targetable again; the copies vanish 8 s later (serpent_part_check_timeout).
void ai_state::serpent_illusion_update()
{
    int i;
    Character *p;

    if (owner->handle == serpent_handle) {
        if (!serpent_illusion_active_timer.valid()) {
            serpent_illusion_active_timer.SetTimeout(3400);
            serpent.illusion_fx_on = false;
            for (i = 0; i < serpent_num_extra_parts + 1; i++) {
                p = living_from_handle(serpent.handles[i]);
                if (p) {
                    p->pos = serpent.part_pos[i];
                    if (i != 0)
                        p->set_ai_active(1);
                    p->find_ground();
                    p->instance->_unkb4 = 0;
                }
            }
        }
        int rem = serpent_illusion_active_timer.GetRemainingTime();
        if (rem > 2000) {
            for (i = 0; i < serpent_num_extra_parts + 1; i++) {
                p = living_from_handle(serpent.handles[i]);
                if (p)
                    p->instance->_unkb4 = 0;
            }
            return;
        }
        if (rem > 300) {
            bool start_fx = !serpent.illusion_fx_on;
            for (i = 0; i < serpent_num_extra_parts + 1; i++) {
                p = living_from_handle(serpent.handles[i]);
                if (p) {
                    if (start_fx)
                        fx_attach_to_character("Serp-illusion.vfx", p->handle, -1, -1, 0, 0);
                    if (i != 0)
                        p->set_ai_active(1);
                }
            }
            serpent.illusion_fx_on = true;
            for (i = 0; i < serpent_num_extra_parts + 1; i++) {
                p = living_from_handle(serpent.handles[i]);
                if (p) {
                    if (rem < 1300) {
                        int base = 255;
                        if (i != 0)
                            base = 230;
                        int a = (rem - 300) * base / -1000 + base;
                        if (a < 1)
                            a = 1;
                        p->instance->_unkb4 = (ubyte)a;
                    } else {
                        p->instance->_unkb4 = 1;
                    }
                }
            }
            return;
        }
        if (rem > 0 && rem <= 300) {
            for (i = 0; i < serpent_num_extra_parts + 1; i++) {
                p = living_from_handle(serpent.handles[i]);
                if (p) {
                    int a = 255;
                    if (i != 0)
                        a = 230;
                    p->instance->_unkb4 = a;
                    p->flags &= ~(CF_UNTARGETABLE | CF_HIDDEN);
                }
            }
            serpent_illusion_timer.SetTimeout(8000);
            return;
        }
        for (i = 0; i < serpent_num_extra_parts + 1; i++) {
            p = living_from_handle(serpent.handles[i]);
            if (p) {
                p->flags &= ~(CF_UNTARGETABLE | CF_HIDDEN);
                p->instance->_unkb4 = i ? 230 : 255;
            }
        }
        serpent_illusion_timer.SetTimeout(8000);
        serpent_illusion_active_timer.invalidate();
    }
    go_idle();
}

// 0x4970a0  Confidence: exact (the retry loop; the success test first, the inner index from i - 1, the
// distance as a vector difference with y cleared, D7700). Sub-mode 0x38 setup: a point within 4 m of the
// serpent for each of the three, at least 2.5 m from the earlier ones (four retries each).
bool ai_state::serpent_place_parts()
{
    bool ok = true;
    int i;

    for (i = 0; i < serpent_num_extra_parts + 1; i++) {
        int tries = 4;
        bool placed;
        do {
            int j;
            placed = true;
            vector3 p;
            p.random_direction();
            p *= 4.0f;
            p += owner->pos;
            pf_find_nearest_navigable_point(&p, owner->info->size, owner->layer);
            serpent.part_pos[i] = p;
            for (j = i - 1; j >= 0; j--) {
                vector3 d = p - serpent.part_pos[j];
                d.y = 0.0f;
                if (d.len_sq() < 6.25f) {
                    placed = false;
                    break;
                }
            }
        } while (!placed && tries-- != 0);
        ok &= placed;
    }
    if (ok) {
        serpent_illusion_active_timer.invalidate();
        serpent_place_timer.SetTimeout(10000);
        return true;
    }
    serpent_place_timer.SetTimeout(500);
    return false;
}

// 0x497230  Confidence: exact (the retry loop runs eight times: `while (tries--)` from 8; the horizontal
// distance is a vector difference with y cleared and len_sq, D7700). A navigable point 7-10 m away with a
// path; the mount travels it (AI mode 11).
bool ai_state::serpent_wander_start()
{
    int tries = 8;
    while (tries--) {
        vector3 p;
        p.random_direction();
        p *= 10.0f;
        p += owner->pos;
        if (pf_point_is_navigable(&p, owner->info->size, owner->layer)) {
            vector3 d = p - owner->pos;
            d.y = 0.0f;
            if (d.len_sq() > 49.0f
                && path_find_new(&owner->pos, owner->layer, &p, owner->layer, owner->info->size, 0.0f)) {
                Character *m = owner->get_mount();
                if (m) {
                    m->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
                    serpent_wander_timer.SetTimeout(5000);
                    return true;
                }
            }
        }
    }
    serpent_timer_b.SetTimeout(500);
    return false;
}

// 0x497370  Confidence: exact. The copies are drawn at alpha 230, the serpent opaque, except while the
// serpent's illusion runs.
void serpent_part_update_alpha(Character *c)
{
    if (character_is_serpent_part(c)) {
        Character *s = living_from_handle(serpent_handle);
        if (s && (s->ai.is_mode(AI_STATE_COMBAT, 0x39) || s->ai.is_mode(AI_STATE_COMBAT, 0x38)))
            return;
    }
    c->instance->_unkb4 = character_is_serpent_part(c) ? 230 : 255;
}

// 0x4973e0  Confidence: exact (the choose result is a switch, D7700; case order from the bodies: 15, 22/23,
// 0x20, 16, 0x38, 0x39; the table starts at -1, which has the default body). Every frame of a serpent part in combat.
void ai_state::serpent_ai_frame()
{
    Character *t = living_from_handle(target);
    if (t && (t->flags & CF_HIDDEN))
        t = NULL;
    int r = serpent_choose_attack();
    switch (r) {
    case 0:
        go_idle();
        return;
    case 1:
        if (serpent.pending_submode == 0x38) {
            if (!serpent_place_parts())
                return;
            set_mode(AI_STATE_COMBAT, 0x38);
        } else if (serpent.pending_submode == 0x20) {
            if (!serpent_wander_start())
                return;
            serpent_timer_b_set_random(15000, 25000);
            set_mode(AI_STATE_COMBAT, 0x20);
        }
        break;
    }
    int part_target = serpent_part_target[serpent_part_index(owner)];
    if (part_target != target) {
        target = part_target;
        owner->combat.target = target;
        flags |= 0x10;
        t = living_from_handle(target);
    }
    if (t == NULL || (t->flags & CF_HIDDEN)) {
        go_idle();
        return;
    }
    attack_range = 1.5f;
    if (r == 3)
        combat_approach(t);
    serpent_part_update_alpha(owner);
    switch (submode) {
    case 15:
        combat_melee_attack(t);
        return;
    case 22:
    case 23:
        combat_close_in(t);
        return;
    case 0x20:
        serpent_wander_update();
        return;
    case 16:
        combat_ranged_attack(t);
        return;
    case 0x38:
        serpent_illusion_start();
        return;
    case 0x39:
        serpent_illusion_update();
        return;
    case -1:
        return;
    }
}

// 0x4975a0  Confidence: exact (VC6 byte match) (the not-found result as -1 mapped to 0). 0 for the serpent and for a
// character that is no part.
int serpent_part_index(Character *c)
{
    int idx = -1;
    for (int i = 0; i < serpent_num_extra_parts + 1; i++) {
        if (c->handle == serpent.handles[i]) {
            idx = i;
            break;
        }
    }
    if (idx == -1)
        idx = 0;
    return idx;
}

// 0x4975e0  Confidence: exact.
void boss_part_deactivate(Character *c)
{
    c->set_ai_active(0);
    c->flags |= CF_UNTARGETABLE | CF_HIDDEN;
    c->ai.go_idle();
}

// 0x497610  Confidence: exact. A visible copy vanishes when the illusion timer runs out.
void ai_state::serpent_part_check_timeout()
{
    if (owner->handle != serpent_handle && !(owner->flags & CF_HIDDEN) && serpent_illusion_timer.elapsed())
        boss_part_deactivate(owner);
}

// ---- animation sets ---------------------------------------------------------------------------------
// 0x497650  Confidence: exact. The dismount (boss_set_foot_anims): stage 3.
void serpent_set_foot_anims(Character *c)
{
    int anim;

    serpent_has_rider = false;
    anim = charinfo_find_anim(c->info, "ready");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
    }
    anim = charinfo_find_anim(c->info, "ready_hurt");
    if (anim != -1) {
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "walk");
    if (anim != -1) {
        c->anim_norm_walk = anim;
        c->anim_walk = anim;
        c->anim_norm_walk_hurt = anim;
        c->anim_walk_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "run");
    if (anim != -1) {
        c->anim_norm_run = anim;
        c->anim_run = anim;
        c->anim_norm_run_hurt = anim;
        c->anim_run_hurt = anim;
    }
    c->anim_tripped_liedown = charinfo_find_anim(c->info, "tripped_liedown");
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
    serpent_stage = 3;
    serpent_target_stage = 3;
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_script_hook_stub case 0x59 (boss_ai.h: folded).
void serpent_script_hook(Character *c)
{
}

// folded (0x49c610, boss_dismount)  Confidence: likely. boss_dismount_dispatch case 0x59: the serpent's copy of
// the rider dismount, identical to the tiger's and the phoenix's; the linker folded the three into the
// tiger unit's copy (D7603 (2), D8902). Body as boss_dismount.
void serpent_dismount(Character *c)
{
    Character *m = c->get_mount();
    if (m == NULL)
        return;
    m->mount_flags = 0;
    m->mount_link = -1;
    m->ai.target = -1;
    m->flags &= ~0x20;
    c->mount_flags = 0;
    c->mount_link = -1;
    c->flags |= CF_COMBAT_READY;
}

// 0x497750  Confidence: exact. boss_set_ride_anims: back on the mount, stage 1.
void serpent_set_ride_anims(Character *c)
{
    int anim;

    c->anim_tripped_liedown = -1;
    anim = charinfo_find_anim(c->info, "ride_horse_ready");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "ride_horse_walk");
    if (anim != -1) {
        c->anim_norm_walk = anim;
        c->anim_walk = anim;
        c->anim_norm_walk_hurt = anim;
        c->anim_walk_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "ride_horse_run");
    if (anim != -1) {
        c->anim_norm_run = anim;
        c->anim_run = anim;
        c->anim_norm_run_hurt = anim;
        c->anim_run_hurt = anim;
    }
    boss_ensure_mount(c);
    serpent_target_stage = 1;
    serpent_stage = 1;
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
    serpent_has_rider = true;
}

// 0x497840  Confidence: exact. cutscene_start_special_cases: the rider walks in its stand slots.
void serpent_set_cutscene_walk_anim()
{
    Character *c = living_from_handle(serpent_handle);
    if (c == NULL)
        return;
    int anim = charinfo_find_anim(c->info, "ride_horse_walk");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
}

// ---- the whip ---------------------------------------------------------------------------------------
// 0x497890  Confidence: exact (VC6 byte match) (the member is not NULL-checked, as in the binary). The party members in
// the whip's reach and angle whose defence roll fails.
bool serpent_whip_test(Character *boss)
{
    serpent_whip_hit_count = 0;
    ai_group *g = cur_character->ai.group;
    if (g) {
        for (int i = 0; i < g->count; i++) {
            Character *m = living_from_handle(g->members[i]);
            float d = boss->distance_between_edges(m, true, -1);
            vector3 dir(m->pos.x - boss->pos.x, 0, m->pos.z - boss->pos.z);
            dir.normalize_safe();
            if (d < serpent_whip_reach
                && dir.x * boss->orient.fvec.x + dir.y * boss->orient.fvec.y + dir.z * boss->orient.fvec.z
                       > serpent_attack_table[2][0].cos_max_angle
                && living_combat_test_defense(&boss->combat, &m->combat)) {
                serpent_whip_hits[serpent_whip_hit_count] = g->members[i];
                serpent_whip_hit_count++;
            }
        }
    }
    return serpent_whip_hit_count > 0;
}

// 0x4979b0  Confidence: exact.
void serpent_whip_resolve_hit(Character *boss)
{
    for (int i = 0; i < serpent_whip_hit_count; i++) {
        Character *c = living_from_handle(serpent_whip_hits[i]);
        if (c)
            living_combat_resolve_hit_vs(&boss->combat, &c->combat);
    }
}

// 0x497a00  Confidence: exact. boss_move_in_range_dispatch.
bool serpent_move_in_range(Character *boss, Character *target)
{
    int idx = serpent_part_index(boss);
    if (idx >= 0 && idx < SERPENT_MAX_PARTS)
        return boss_move_target_in_range(boss, target, serpent_part_busy[idx]);
    return false;
}

// 0x497a40  Confidence: exact. cmd_serpent_solo, the level scripts: on foot in stage 3, the two parts
// deleted, combat on.
void serpent_set_solo(Character *rider)
{
    rider_unlink_mount(rider);
    rider->flags |= CF_COMBAT_READY;
    serpent_set_foot_anims(rider);
    rider->process_anim(0.0f);
    rider->find_ground();
    for (int i = 0; i < 2; i++) {
        Character *p = living_from_handle(serpent_part_handles[i]);
        if (p)
            p->obj_flags = (p->obj_flags & ~1) | 4;
    }
    serpent_combat = true;
    serpent_stage = 3;
    serpent_target_stage = 3;
}
