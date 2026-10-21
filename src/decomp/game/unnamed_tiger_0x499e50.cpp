// unnamed_tiger_0x499e50.cpp - Sum.exe v1.40, code 0x499e50-0x49c83a: the Tiger Rider boss (Liangshan,
// Jadetemple and the TempleInt level): the three weapons stages (spear, axe, on foot), the target
// choice, the charge (sub-mode 0x1f), the reposition and wander moves, the console commands
// tiger_stage / tiger_unlink / tiger_combat / tiger_solo, and boss_dismount (used for all three
// riders).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: init_tiger_timers (entry 283, 0x499e50) after the phoenix unit's phoenix_evaluate_target,
// up to tiger_set_solo (0x49c7c0-0x49c83a); pathfinding.cpp starts at 0x49c840.
// Data: .bss 0x2452a70-0x2452cb7 (ccmd_tiger_solo, ccmd_tiger_stage, the fight state object 0x2452a90-
// 0x2452c97 (D3403), ccmd_tiger_combat, tiger_mount, ccmd_tiger_unlink, tiger_wander_misses), .data
// 0x59c980-0x59cb27 (console strings, the animation names, "TigerSpear"). "TigerAxe" (0x5994bc) and the
// float constants are pooled; 3.5 (0x581b28) is this unit's.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/ai/unnamed_tiger_0x499e50.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/math/ix.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"

#define SPELL_TIGER_ROAR 0x49         // hits the target at the start of the charge and the wander (name ours)
#define TIGER_CHARGE_MOVE 0xb8        // the charge move id (tiger_cache_move_b8, living_combat::update)

// ---- .bss ------------------------------------------------------------------------------------------
// 0x499e50 init_tiger_timers_thunk (jmp, static initialiser entry 283). Confidence: exact.
// 0x499e60 init_tiger_timers: the compiler-generated constructor of tiger_boss (the five threat timers,
//          the reposition timer, the target timer). Confidence: exact.
tiger_boss tiger;                                             // 0x2452a90
Character *tiger_mount;                                       // 0x2452ca4
int tiger_wander_misses;                                      // 0x2452cb4

// ---- console commands -------------------------------------------------------------------------------
// 0x499e90 init_ccmd_tiger_stage_thunk (jmp, static initialiser entry 284). Confidence: exact.
// 0x499ea0 init_ccmd_tiger_stage (console_command constructor). Confidence: exact.
console_command ccmd_tiger_stage("tiger_stage", "Set Tiger rider's current combat stage", cmd_tiger_stage);   // 0x2452a80

// 0x499ec0  Confidence: exact.
void cmd_tiger_stage()
{
    process_command(ARG_INT);
    if (arg_type & ARG_INT) {
        if (command_argument_int >= 0 && command_argument_int <= 3)
            tiger_target_stage = command_argument_int;
        else
            print_to_console("Invalid stage for tiger rider\n", 0);
    }
}

// 0x499f00 init_ccmd_tiger_unlink_thunk (jmp, static initialiser entry 285). Confidence: exact.
// 0x499f10 init_ccmd_tiger_unlink (console_command constructor). Confidence: exact.
console_command ccmd_tiger_unlink("tiger_unlink", "Unlinks tiger to mount", cmd_tiger_unlink);   // 0x2452ca8

// 0x499f30  Confidence: exact (VC6 byte match) (the store order). The last tiger rider in the living list.
void cmd_tiger_unlink()
{
    Character *rider = NULL;
    for (Character *c = living_list.live_next; c != &living_list; c = c->live_next)
        if (c->info->species == SPECIES_TIGER_RIDER)
            rider = c;
    if (rider == NULL)
        return;
    Character *m = rider->get_mount();
    if (m == NULL)
        return;
    m->mount_flags = 0;
    m->mount_link = -1;
    m->ai.target = -1;
    m->flags &= ~0x20;
    rider->mount_flags = 0;
    rider->mount_link = -1;
    rider->flags |= CF_COMBAT_READY;
}

// 0x499fc0 init_ccmd_tiger_combat_thunk (jmp, static initialiser entry 286). Confidence: exact.
// 0x499fd0 init_ccmd_tiger_combat (console_command constructor). Confidence: exact.
console_command ccmd_tiger_combat("tiger_combat", "Sets tiger in combat state", cmd_tiger_combat);   // 0x2452c98

// 0x499ff0  Confidence: exact.
void cmd_tiger_combat()
{
    tiger_combat = true;
    print_to_console("Setting tiger's ai state to combat\n", 0);
}

// 0x49a010  Confidence: exact. The last match (cmd_tiger_solo, the TempleInt script).
Character *tiger_find_rider()
{
    Character *found = NULL;
    for (Character *c = living_list.live_next; c != &living_list; c = c->live_next)
        if (c->info->species == SPECIES_TIGER_RIDER)
            found = c;
    return found;
}

// 0x49a050 init_ccmd_tiger_solo_thunk (jmp, static initialiser entry 287). Confidence: exact.
// 0x49a060 init_ccmd_tiger_solo (console_command constructor). Confidence: exact.
console_command ccmd_tiger_solo("tiger_solo", "Set tiger without horse\n", cmd_tiger_solo);   // 0x2452a70

// 0x49a080  Confidence: exact.
void cmd_tiger_solo()
{
    Character *c = tiger_find_rider();
    if (c)
        tiger_set_solo(c);
}

// ---- moves and animation sets ------------------------------------------------------------------------
// 0x49a0a0  Confidence: exact.
int tiger_current_move_index()
{
    if (tiger_cur_move) {
        int i = tiger_cur_move - tiger_move_table;
        if (i >= 0 && i < TIGER_NUM_MOVES)
            return i;
    }
    return -1;
}

// 0x49a0c0  Confidence: exact. The ride stand / walk / run in the ready slots (the stand slots keep theirs).
void tiger_set_ride_stand_anims(Character *c)
{
    int stand = tiger_anim_ride_stand;
    c->anim_ready = stand;
    c->anim_ready_hurt = stand;
    int walk = tiger_anim_ride_walk;
    c->anim_walk = walk;
    c->anim_walk_hurt = walk;
    int run = tiger_anim_ride_run;
    c->anim_run = run;
    c->anim_run_hurt = run;
}

// 0x49a100  Confidence: exact.
void tiger_set_stage1_anims(Character *c)
{
    int ready = tiger_anim_ride_ready;
    c->anim_ready = ready;
    c->anim_ready_hurt = ready;
    int walk = tiger_anim_stage1_walk;
    c->anim_walk = walk;
    c->anim_walk_hurt = walk;
    int run = tiger_anim_stage1_run;
    c->anim_run = run;
    c->anim_run_hurt = run;
}

// 0x49a140  Confidence: exact. The stand slots get stage2_walk too (ride_horse_ready_axe, loaded into
// tiger.anim_ride_ready_axe, is never used; NOTES.md).
void tiger_set_stage2_anims(Character *c)
{
    int stand = tiger_anim_stage2_walk;
    c->anim_stand = stand;
    c->anim_ready = stand;
    c->anim_stand_hurt = stand;
    c->anim_ready_hurt = stand;
    int walk = tiger_anim_stage2_walk;
    c->anim_norm_walk = walk;
    c->anim_walk = walk;
    c->anim_norm_walk_hurt = walk;
    c->anim_walk_hurt = walk;
    int run = tiger_anim_stage2_run;
    c->anim_norm_run = run;
    c->anim_run = run;
    c->anim_norm_run_hurt = run;
    c->anim_run_hurt = run;
}

// 0x49a1a0  Confidence: exact (VC6 byte match) (the buffer sizes). As serpent_rider_set_flinch.
void tiger_rider_set_flinch(move_info *moves)
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
            tiger.flinch_area[stage] = m;
        else if (stricmp(p, "status") == 0)
            tiger.flinch_status[stage] = m;
        else
            sprintf(msg, "Unable to set Tiger Rider flinch for %s -- bad $Action\n", m->anim_file);
    }
    tiger.flinch_area[0] = tiger.flinch_area[1];      // stage 2 into stage 1 (0x49a29d-0x49a2ad)
    tiger.flinch_status[0] = tiger.flinch_status[1];
}

// 0x49a2c0  Confidence: exact (VC6 byte match) (the two equip results land in the argument slots). Stage 1: mounted
// (unless on_foot), the spear on four tags and the axe on the fifth, four spear moves left.
void tiger_equip_weapon(Character *c, bool on_foot)
{
    int anim;

    if (!on_foot) {
        boss_ensure_mount(c);
        anim = tiger_anim_ride_stand;
        if (anim != -1) {
            c->anim_stand = anim;
            c->anim_ready = anim;
            c->anim_stand_hurt = anim;
            c->anim_ready_hurt = anim;
        }
        anim = tiger_anim_ride_walk;
        if (anim != -1) {
            c->anim_norm_walk = anim;
            c->anim_walk = anim;
            c->anim_norm_walk_hurt = anim;
            c->anim_walk_hurt = anim;
        }
        anim = tiger_anim_ride_run;
        if (anim != -1) {
            c->anim_norm_run = anim;
            c->anim_run = anim;
            c->anim_norm_run_hurt = anim;
            c->anim_run_hurt = anim;
        }
    }
    int spear = item_type_find("TigerSpear");
    item_info *weapon = item_create_weapon(spear);
    tiger_weapon = weapon;
    int axe = item_type_find("TigerAxe");
    tiger_axe = item_create_weapon(axe);
    item_info *old1, *old2;
    character_equip_item(&old1, &old2, c, 5, weapon);
    if (old1)
        item_free(old1);
    if (old2)
        item_free(old2);
    c->attachments[0].item_type = spear;
    c->attachments[1].item_type = spear;
    c->attachments[2].item_type = spear;
    c->attachments[3].item_type = spear;
    c->attachments[0].slot = 0xb;
    c->attachments[1].slot = 0xc;
    c->attachments[2].slot = 0xd;
    c->attachments[3].slot = 0xe;
    c->attachments[4].item_type = axe;
    c->attachments[4].slot = 4;
    c->num_attachments = 5;
    tiger_spear_uses_left = 4;
    tiger_target = -1;
}

// 0x49a420  Confidence: exact (VC6 byte match). Stage 2: the axe in hand, the four spear tags emptied (slot 0x11).
void tiger_swap_weapon(Character *c)
{
    if (tiger_axe) {
        item_info *old1, *old2;
        character_equip_item(&old1, &old2, c, 5, tiger_axe);
        tiger_axe = NULL;
        if (old1)
            item_free(old1);
        if (old2)
            item_free(old2);
    }
    c->attachments[4].slot = 4;
    c->attachments[0].slot = 0x11;
    c->attachments[1].slot = 0x11;
    c->attachments[2].slot = 0x11;
    c->attachments[3].slot = 0x11;
    c->num_attachments = 5;
    tiger_spear_uses_left = 0;
}

// 0x49a4b0  Confidence: exact (VC6 byte match). Stage 3, on foot: the walking animations, the axe in hand (slot 0), off
// the mount.
void tiger_setup_anims(Character *c)
{
    int anim;

    anim = tiger_anim_ready;
    if (anim != -1) {
        c->anim_ready = anim;
        c->anim_stand = anim;
    }
    anim = tiger_anim_ready_hurt;
    if (anim != -1) {
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = tiger_anim_walk;
    if (anim != -1) {
        c->anim_norm_walk = anim;
        c->anim_walk = anim;
        c->anim_norm_walk_hurt = anim;
        c->anim_walk_hurt = anim;
    }
    anim = tiger_anim_run;
    if (anim != -1) {
        c->anim_norm_run = anim;
        c->anim_run = anim;
        c->anim_norm_run_hurt = anim;
        c->anim_run_hurt = anim;
    }
    if (tiger_axe) {
        item_info *old1, *old2;
        character_equip_item(&old1, &old2, c, 5, tiger_axe);
        tiger_axe = NULL;
        if (old1)
            item_free(old1);
        if (old2)
            item_free(old2);
    }
    c->attachments[4].slot = 0;
    c->attachments[0].slot = 0x11;
    c->attachments[1].slot = 0x11;
    c->attachments[2].slot = 0x11;
    c->attachments[3].slot = 0x11;
    c->num_attachments = 5;
    tiger_spear_uses_left = 0;
    boss_release_mount(c);
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
}

// 0x49a5e0  Confidence: exact. boss_set_ride_anims: back on the mount, stage 2.
void tiger_set_ride_anims(Character *c)
{
    int anim;

    anim = charinfo_find_anim(c->info, "ride_horse_stand");
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
    tiger_target_stage = 2;
    tiger_stage = 2;
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
    tiger_has_rider = true;
}

// 0x49a6c0  Confidence: exact (VC6 byte match) (the case order of the forced stage). A forced stage, else below 50 % HP
// stage 3 (after the current move and the level script's answer), above 50 % with the spear used up
// stage 2, else stage 1.
void ai_state::tiger_check_stage()
{
    if (tiger_target_stage != 0) {
        if (tiger_stage == tiger_target_stage)
            return;
        switch (tiger_target_stage) {
        default:
            tiger_target_stage = 0;
            return;
        case 3:
            tiger_setup_anims(owner);
            tiger_stage = 3;
            return;
        case 2:
            tiger_swap_weapon(owner);
            tiger_stage = 2;
            return;
        case 1:
            tiger_equip_weapon(owner, false);
            tiger_stage = 1;
            return;
        }
    }
    Character *c = owner;
    float frac = c->hp / (float)c->get_max_stat(0);
    if (frac < 0.5f) {
        if (tiger_stage == 3)
            return;
        if (!owner->move_finished(owner->cur_anim))
            return;
        if (!tiger_dismount_stage_reached) {
            tiger_dismount_stage_reached = true;
            tiger_stage_trigger = false;
            return;
        }
        if (tiger_stage_trigger) {
            tiger_setup_anims(owner);
            tiger_stage = 3;
        }
        return;
    }
    if (frac > 0.5f && tiger_spear_uses_left == 0) {
        if (tiger_stage != 2) {
            tiger_swap_weapon(owner);
            tiger_stage = 2;
        }
        return;
    }
    if (tiger_stage != 1) {
        tiger_equip_weapon(owner, false);
        tiger_stage = 1;
    }
}

// 0x49a830  Confidence: exact (dir scaled and moved in place; the dot product written x, z, y, D10308). A random horizontal
// direction whose dot product with (target - self) lies in [dot_min, dot_max], at rmin..rmax from the
// target, more than min_dist from self and navigable.
bool tiger_pick_wander_point(Character *self, Character *target, int tries, float rmin, float rmax,
                             float dot_min, float dot_max, float min_dist)
{
    vector3 to(target->pos.x - self->pos.x, target->pos.y - self->pos.y, target->pos.z - self->pos.z);
    while (tries-- != 0) {
        vector3 dir;
        dir.random_direction();
        dir.y = 0;
        dir.normalize_safe();
        float dot = dir.x * to.x + to.z * dir.z + to.y * dir.y;
        if (dot < dot_min)
            continue;
        if (dot > dot_max)
            continue;
        dir *= frand_range(rmin, rmax);
        dir += target->pos;
        if ((dir - self->pos).len() <= min_dist)
            continue;
        if (pf_point_is_navigable(&dir, self->info->size, self->layer)) {
            tiger_wander_point = dir;
            return true;
        }
    }
    return false;
}

// 0x49a9b0  Confidence: exact (VC6 byte match) (the direction to the target is computed and normalised but not used).
// Three tries at a wander point behind and beside the tiger relative to the target, 8-12 m from it.
bool ai_state::tiger_wander_start(int target_handle)
{
    Character *t = living_from_handle(target_handle);
    if (t == NULL)
        return false;
    if (tiger_stage == 3)
        return false;
    vector3 dir;
    dir = t->pos - owner->pos;
    dir.normalize_safe_length();
    bool ok = false;
    for (int i = 0; i < 3; i++) {
        if (tiger_pick_wander_point(owner, t, 10, 8.0f, 12.0f, -0.707f, -0.1f, 5.0f)
            && path_find_new(&owner->pos, owner->layer, &tiger_wander_point, owner->layer, owner->info->size, 0.0f)) {
            Character *m = owner->get_mount();
            if (m)
                m->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
            ok = true;
            break;
        }
    }
    return ok;
}

// 0x49aad0  Confidence: exact. The six move records hold only their variation.
void tiger_init_move_table()
{
    tiger_move_table[0].variation = 0xb6;
    tiger_move_table[1].variation = 0xb8;
    tiger_move_table[2].variation = 0xb9;
    tiger_move_table[3].variation = 3;
    tiger_move_table[4].variation = 2;
    tiger_move_table[5].variation = 1;
}

// 0x49ab10  Confidence: exact.
void tiger_load_anims(Character *c)
{
    tiger_anim_ride_stand = charinfo_find_anim(c->info, "ride_horse_stand");
    tiger_anim_ride_ready = charinfo_find_anim(c->info, "ride_horse_ready");
    tiger_anim_ride_walk = charinfo_find_anim(c->info, "ride_horse_walk");
    tiger_anim_stage1_walk = charinfo_find_anim(c->info, "tigr_stage1_walk");
    tiger_anim_ride_run = charinfo_find_anim(c->info, "ride_horse_run");
    tiger_anim_stage1_run = charinfo_find_anim(c->info, "tigr_stage1_run");
    tiger.anim_ride_ready_axe = charinfo_find_anim(c->info, "ride_horse_ready_axe");
    tiger_anim_stage2_walk = charinfo_find_anim(c->info, "tigr_stage2_walk");
    tiger_anim_stage2_run = charinfo_find_anim(c->info, "tigr_stage2_run");
    tiger_anim_ready = charinfo_find_anim(c->info, "ready");
    tiger_anim_ready_hurt = charinfo_find_anim(c->info, "ready_hurt");
    tiger_anim_walk = charinfo_find_anim(c->info, "walk");
    tiger_anim_run = charinfo_find_anim(c->info, "run");
}

// 0x49ac40  Confidence: exact.
void tiger_reset_b58()
{
    tiger.reposition_state = -1;
    tiger.reposition_failures = 0;
}

// 0x49ac60  Confidence: exact.
void tiger_reset_b50()
{
    tiger.wander_state = -1;
    tiger.wander_failures = 0;
}

// 0x49ac80  Confidence: exact.
void tiger_reset_b54()
{
    tiger.state_b54 = -1;
}

// 0x49ac90  Confidence: exact. The charge is next.
void tiger_reset_move_state()
{
    tiger.charge_state = -1;
    tiger_cur_move = &tiger_move_table[1];
    tiger.charging = false;
    tiger_move_trigger_pending = false;
    tiger.charge_fired = false;
}

// 0x49acc0  Confidence: exact. The charge move and the time of its trigger 7 (0.4 s without one).
void tiger_cache_move_b8(Character *c)
{
    float t = 0.4f;
    move_info *m = c->find_move(0, TIGER_CHARGE_MOVE);
    tiger_cached_move = m;
    if (m) {
        for (int i = 0; i < 3; i++) {
            if (m->triggers[i].id == 7) {
                t = m->triggers[i].time;
                break;
            }
        }
    }
    tiger.charge_trigger_time = t;
}

// ---- setup -----------------------------------------------------------------------------------------
// 0x49ad20  Confidence: exact (VC6 byte match). Every load: the stage (on foot, stage 3 in Liangshan / Jadetemple), the
// move table, the animations, the weapons, the flinch moves, the timers and move state.
void ai_state::tiger_boss_load()
{
    if (stricmp(level_filename, "Liangshan") == 0 || stricmp(level_filename, "Jadetemple") == 0) {
        tiger_stage = 3;
        tiger_dismount_stage_reached = false;
        tiger_stage_trigger = false;
        tiger_target_stage = 3;
        tiger_has_rider = false;
    } else {
        tiger_stage = 1;
        tiger_dismount_stage_reached = false;
        tiger_stage_trigger = false;
        tiger_target_stage = 0;
        tiger_has_rider = true;
    }
    for (int i = 0; i < BOSS_MAX_THREATS; i++)
        tiger_threat_table[i].handle = -1;
    tiger_init_move_table();
    tiger.move_b78 = charinfo_find_move(owner->info, 0xb, 0x20, true);
    tiger_cache_move_b8(owner);
    tiger_load_anims(owner);
    if (stricmp(level_filename, "Liangshan") != 0 && stricmp(level_filename, "Jadetemple") != 0) {
        tiger_equip_weapon(owner, false);
    } else {
        tiger_equip_weapon(owner, true);
        tiger_setup_anims(owner);
    }
    tiger_rider_set_flinch((move_info *)owner->info->moves[4]);
    tiger.target_timer.SetTimeout(0);
    tiger_combat = false;
    tiger_cur_move = NULL;
    tiger.weapon_ready = false;
    tiger_reset_b58();
    tiger_reset_b50();
    tiger_reset_b54();
    tiger_reset_move_state();
    tiger_mount = owner->get_mount();
}

// 0x49ae90  Confidence: exact. ai_state_shutdown: the axe not handed over yet (the pointer is kept).
void tiger_unload()
{
    if (tiger_axe)
        item_free(tiger_axe);
}

// 0x49aeb0  Confidence: exact. As serpent_seat_rider (D7700).
bool ai_state::tiger_seat_rider()
{
    if (tiger_mount)
        boss_rider_random_sound(&tiger_mount->pos);
    if (tiger_has_rider) {
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

// ---- target and attack choice -------------------------------------------------------------------------
// 0x49b040  Confidence: exact.
void ai_state::tiger_ai_idle()
{
    if (owner->get_current_move() == NULL || owner->combat.state == 0) {
        owner->combat.state = 0;
        if (tiger_combat == true)
            enter_combat();
    }
}

// 0x49b080  Confidence: exact.
int tiger_threat_find(int handle)
{
    for (int i = 0; i < BOSS_MAX_THREATS; i++)
        if (tiger_threat_table[i].handle == handle)
            return i;
    return -1;
}

// One candidate of tiger_pick_target (0x10 bytes, five on its stack). Name ours.
struct tiger_candidate {
    Character *c;                     // +0x0
    int _unk4;                        // +0x4
    bool ok;                          // +0x8 seen (ai_score_target) or a threat less than 20 s old
    float score;                      // +0xc cumulative weight
};

// 0x49b0a0  Confidence: likely (the threat base is assigned per candidate, not summed; NOTES.md). A
// party member the tiger sees or that hurt it recently, weighted by its threat (the only one when there
// is one).
int tiger_pick_target(Character *boss)
{
    tiger_candidate cand[5];
    ai_group *g = cur_character->ai.group;
    int i, k;
    int count = 0;

    memset(cand, 0, sizeof(cand));
    if (g == NULL)
        return -1;
    for (i = 0; i < g->count; i++) {
        Character *c = living_from_handle(g->members[i]);
        cand[i].c = c;
        cand[i].ok = false;
        if (c == NULL)
            continue;
        ai_target_query q;
        q._unk08 = 2;
        q.result = 0;
        q.best = boss->info->fov_range;
        q.self = boss;
        q.candidate = c;
        ai_score_target(&q, 1);
        bool seen = q.best < boss->info->fov_range;
        k = tiger_threat_find(c->handle);
        if ((k != -1 && !tiger_threat_table[k].timer.elapsed()) || seen) {
            cand[i].ok = true;
            count++;
        }
    }
    if (count == 0)
        return -1;
    if (count == 1) {
        for (i = 0; i < g->count; i++)
            if (cand[i].ok == true)
                return cand[i].c->handle;
        return -1;
    }
    float base_threat = 10.0f;
    for (i = 0; i < g->count; i++) {
        if (cand[i].ok == true) {
            k = tiger_threat_find(cand[i].c->handle);
            if (k != -1)
                base_threat = -(tiger_threat_table[k].threat[1] + tiger_threat_table[k].threat[0]);
        }
    }
    float total = 0;
    for (i = 0; i < g->count; i++) {
        float w = 0.0f;
        if (cand[i].ok == true) {
            float base = base_threat * 0.1f + 10.0f;
            k = tiger_threat_find(cand[i].c->handle);
            if (k != -1)
                w = base - (tiger_threat_table[k].threat[0] * 3.0f + tiger_threat_table[k].threat[1]);
            else
                w = base;
        }
        total += w;
        cand[i].score = total;
    }
    for (i = 0; i < g->count; i++)
        cand[i].score /= total;
    float r = frand();
    for (i = 0; i < g->count; i++)
        if (r < cand[i].score)
            return cand[i].c->handle;
    return -1;
}

// 0x49b350  Confidence: exact (one condition on submode read in place; the stage 3 cases break to one return, D10305). 5 while a move of the
// table or a special sub-mode runs, else the next move: stage 1 the spear (0), stage 2 the charge (1)
// or the axe (2), stage 3 one of 3-5 at random; 0 in other stages.
int ai_state::tiger_choose_attack()
{
    int p;

    if (submode == -1) {
        if (tiger_target == -1)
            tiger_target = tiger_pick_target(owner);
        owner->combat.state = 0;
        tiger_cur_move = NULL;
    }
    if (owner->combat.is_busy() == true)
        return tiger_cur_move ? 5 : 0;
    if (submode == 0x20 || (submode == 0x21 && tiger.wander_state != -1) || submode == 0x1f || submode == 0x1e)
        return 5;
    if (tiger_current_move_index() != -1)
        return 5;

    if (tiger_stage == 1) {
        p = tiger_pick_target(owner);
        if (p != -1)
            tiger_target = p;
        tiger_cur_move = &tiger_move_table[0];
        return 3;
    }
    if (tiger_stage == 2) {
        bool repick = true;
        if (frand() > 0.7f) {
            tiger_cur_move = &tiger_move_table[1];
        } else {
            tiger_cur_move = &tiger_move_table[2];
            repick = frand() > 0.8f;
        }
        if (repick) {
            p = tiger_pick_target(owner);
            if (p != -1)
                tiger_target = p;
        }
        return 3;
    }
    if (tiger_stage == 3) {
        if (tiger.target_timer.elapsed()) {
            p = tiger_pick_target(owner);
            if (p != -1) {
                tiger_target = p;
                tiger.target_timer.SetRandomTimeout(10000, 15000);
            } else {
                tiger.target_timer.SetTimeout(300);
            }
        }
        switch (rand() % 3) {
        case 0:
            tiger_cur_move = &tiger_move_table[3];
            break;
        case 1:
            tiger_cur_move = &tiger_move_table[4];
            break;
        case 2:
            tiger_cur_move = &tiger_move_table[5];
            break;
        }
        return 3;
    }
    return 0;
}

// 0x49b520  Confidence: unsure (the vector temporaries: the vector3 constructors and copies are called
// out of line here). A path for self to a point 3.5 m past the point 0.75 m beside the target; NULL when
// the target is closer than 3 m.
ai_path *tiger_path_to_target(Character *self, Character *t)
{
    vector3 d(t->pos.x - self->pos.x, 0, t->pos.z - self->pos.z);
    if (d.normalize_safe_length() < 3.0f)
        return NULL;
    vector3 side(-d.z * 0.75f + t->pos.x, t->pos.y, d.x * 0.75f + t->pos.z);
    vector3 dir = side - self->pos;
    dir.y = 0;
    dir.normalize();
    vector3 goal = dir * 3.5f + side;
    return self->ai.path_find_new(&self->pos, self->layer, &goal, t->layer, self->info->size, 0.0f);
}

// 0x49b6e0  Confidence: unsure (the closing-speed arithmetic is transcribed from the x87 code; see
// NOTES.md; D15604: nested ifs with one go_idle at the end give the binary's block layout, fire still
// differs). Sub-mode 0x1f, the mounted charge: the roar at the start, the mount follows its path, the
// strike when the target is close enough for the time left to the move's trigger, then the melee update
// and the attack slots of both sides.
void ai_state::tiger_combat_update(Character *t)
{
    int charge_move = TIGER_CHARGE_MOVE;

    if (tiger.charge_state == 4)
        go_idle();
    Character *m = owner->get_mount();
    if (m != NULL) {
        if (tiger.charge_state == -1) {
            t->get_hit_by_spell(SPELL_TIGER_ROAR, owner->handle, NULL, NULL, 0, NULL, 0.0f, false);
            tiger.charge_state = 2;
        }
        if (tiger.charge_state == 2) {
            if (!follow_path(0)) {
                if (tiger.charging == true && owner->get_current_move() == tiger_cached_move && owner->combat.state != 0) {
                    // living_combat::update takes the move id in its float-typed argument (D3404).
                    owner->combat.update(charge_move);
                    m->ai.go_mount_idle();
                    return;
                }
            } else {
                bool fire = false;
                if (tiger.charging == false || (tiger.charging == true && !tiger.charge_fired)) {
                    vector3 self = owner->pos;
                    self.y = 0;
                    vector3 dest = m->ai.path->pos;
                    vector3 closest;
                    dest.y = 0;
                    float dist = ix_point_linesegment(&t->pos, &self, &dest, &closest);
                    float trigger_time = tiger.charge_trigger_time;
                    float accel = m->info->speedup_rate * m->speed_scale;
                    float v = m->velocity.len();
                    float disc = v * v + accel * dist * 2.0f;
                    float reach;
                    if (disc > 100.0f) {
                        float coast = dist - (100.0f - v * v) / (accel * 2.0f);
                        if (coast * 0.1f > trigger_time) {
                            reach = trigger_time * 10.0f;
                        } else {
                            float accel_time = trigger_time - coast * 0.1f;
                            reach = ((10.0f - accel * accel_time) + 10.0f) * accel_time * 0.5f + coast;
                        }
                    } else {
                        float vmax = (float)sqrt(disc);
                        float w = vmax - trigger_time * accel;
                        reach = (vmax * vmax - w * w) / (accel * 2.0f);
                    }
                    if (tiger.charging == false) {
                        if (reach < dist)
                            return;
                        tiger.charging = true;
                        goto strike;
                    }
                    if (!tiger.charge_fired && dist <= 0.01f)
                        fire = true;
                }
                if (!tiger.charge_fired && fire == true) {
                    tiger.charge_fired = fire;
                    tiger_move_trigger_pending = fire;
                }
            strike:
                ai_move_flags |= 4;
                owner->combat.update(charge_move);
                claim_attack_slot(owner, t, 0, -1);
                t->ai.claim_attack_slot(t, owner, 0, -1);
                t->ai.alert_guards_of_target(owner);
                return;
            }
        }
    }
    go_idle();
}

// 0x49b9e0  Confidence: exact (VC6 byte match). Sub-mode 0x1e: a charge (sub-mode 0x1f, the mount travels) when the
// target is more than 4 m away and a direct path exists, else the axe move and the normal approach.
void ai_state::tiger_approach_target(Character *t)
{
    ai_path *p;
    if (owner->distance_between_edges(t, false, -1) > 4.0f && (p = tiger_path_to_target(owner, t)) != NULL
        && p->next == NULL) {
        Character *m = owner->get_mount();
        if (m) {
            m->ai.path_flags |= 8;
            m->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
        }
        tiger_reset_move_state();
        set_mode(AI_STATE_COMBAT, 0x1f);
        return;
    }
    tiger_cur_move = &tiger_move_table[2];
    combat_approach(t);
}

// 0x49ba70  Confidence: exact (VC6 8966 byte match, m-1020: centre_offset + pos, the inline add's operand order,
// D11905) (the case order: the table starts at -1). Sub-mode 0x21 steps: start,
// pick a wander point (three failures end it), travel, roar, turn to the target, attack.
void ai_state::tiger_wander_update()
{
    if (tiger_stage == 3)
        go_idle();
    Character *t = living_from_handle(target);
    switch (tiger.wander_state) {
    case 0:
        tiger.wander_state = 1;
        tiger_reposition_timer.SetTimeout(0);
    case 1:
        if (tiger_reposition_timer.elapsed()) {
            if (!tiger_wander_start(target)) {
                tiger_reposition_timer.SetTimeout(300);
                if (++tiger.wander_failures >= 3) {
                    go_idle();
                    return;
                }
            } else {
                tiger.wander_state = 2;
                tiger_reposition_timer.SetTimeout(0);
            }
        }
        break;
    case 2:
        if (!follow_path(0)) {
            Character *m = owner->get_mount();
            if (m)
                m->ai.go_mount_idle();
            if ((tiger_wander_point - owner->pos).len() > 1.5f)
                tiger_wander_misses++;
            if (tiger_reposition_timer.GetElapsedTime() > 3000)
                tiger_wander_misses++;
            tiger_reposition_timer.SetTimeout(0);
            tiger.wander_state = 3;
        }
        break;
    case 3:
        t->get_hit_by_spell(SPELL_TIGER_ROAR, owner->handle, NULL, NULL, 0, NULL, 0.0f, false);
        tiger.wander_state = 4;
        tiger_reposition_timer.SetTimeout(0);
    case 4: {
        vector3 centre = t->centre_offset + t->pos;
        Character *m = owner->get_mount();
        bool turning;
        if (m)
            turning = m->ai.turn_towards(&centre, 1);
        else
            turning = turn_towards(&centre, 1);
        if (!turning) {
            tiger.wander_state = 5;
            tiger_reposition_timer.SetTimeout(0);
        }
        break;
    }
    case 5:
        if (tiger_stage != 1)
            break;
        if (!tiger.weapon_ready) {
            set_mode(AI_STATE_COMBAT, 0xe);
            tiger.wander_state = 6;
            return;
        }
        tiger.wander_state = 6;
        if (!combat_approach(t))
            go_idle();
        break;
    case -1:
        go_idle();        // the binary jumps to case 5's go_idle (one shared body)
        break;
    }
}

// 0x49bce0  Confidence: exact (the -1 reset before the state test; a found path jumps past the failure
// count, which follows the loop unconditionally, D7700). Sub-mode 0x20: a wander point (three tries),
// the travel, then the next move (the weapon move 0xe when the weapon is not in hand, the charge approach
// for move 1, else the normal approach); after three failed reposition rounds the wander sub-mode 0x21.
void ai_state::tiger_reposition_update(Character *t)
{
    int s = tiger.reposition_state;
    if (s == 2) {
        tiger_cur_move = NULL;
        return;
    }
    if (s == -1)
        tiger.reposition_state = 0;
    if (s == -1 || s == 0) {
        int i;
        for (i = 0; i < 3; i++) {
            if (tiger_pick_wander_point(owner, t, 10, 8.0f, 12.0f, -0.707f, -0.1f, 5.0f)
                && path_find_new(&owner->pos, owner->layer, &tiger_wander_point, owner->layer, owner->info->size, 0.0f)) {
                Character *m = owner->get_mount();
                if (m)
                    m->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
                tiger.reposition_state = 1;
                goto picked;
            }
        }
        tiger.reposition_failures++;
    }
picked:
    int idx = tiger_current_move_index();
    if (tiger.reposition_state == 0) {
        if (tiger.reposition_failures < 3)
            return;
        if (idx == 0) {
            tiger.wander_failures = idx;
            tiger_reposition_timer.SetTimeout(idx);
            tiger.wander_state = idx;
            set_mode(AI_STATE_COMBAT, 0x21);
            return;
        }
        if (!tiger.weapon_ready) {
            set_mode(AI_STATE_COMBAT, 0xe);
            return;
        }
        if (idx == 1) {
            tiger_reset_b54();
            set_mode(AI_STATE_COMBAT, 0x1e);
            return;
        }
        combat_approach(t);
        return;
    }
    if (tiger.reposition_state == 1) {
        if (follow_path(0))
            return;
        Character *m = owner->get_mount();
        if (m)
            m->ai.go_mount_idle();
        if (idx != 0) {
            if (!tiger.weapon_ready) {
                set_mode(AI_STATE_COMBAT, 0xe);
                tiger.reposition_state = 2;
                return;
            }
            if (idx == 1) {
                tiger_reset_b54();
                set_mode(AI_STATE_COMBAT, 0x1e);
                tiger.reposition_state = 2;
                return;
            }
        }
        combat_approach(t);
        tiger.reposition_state = 2;
    }
}

// 0x49bed0  Confidence: likely (case order from the bodies: 14, 15, 22/23, 16, 0x1f, 0x1e, 0x21, 0x20;
// the table starts at -1, which returns at once). The tiger's own target (re-picked every 0.3 s while it
// has none), the move choice, the sub-mode, and after the weapon move the next one.
void ai_state::tiger_ai_frame()
{
    Character *t = living_from_handle(tiger_target);
    if (t == NULL || (t->flags & CF_HIDDEN)) {
        if (tiger.target_timer.GetRemainingTime() > 300)
            tiger.target_timer.SetTimeout(0);
        if (tiger.target_timer.elapsed()) {
            tiger.target_timer.SetTimeout(300);
            tiger_target = tiger_pick_target(owner);
            t = living_from_handle(tiger_target);
        }
    }
    if (t == NULL || (t->flags & CF_HIDDEN)) {
        go_idle();
        return;
    }
    int idx;
    switch (tiger_choose_attack()) {
    case 0:
        go_idle();
        return;
    case 3:
        idx = tiger_current_move_index();
        if (idx == 0) {
wander:
            tiger.wander_failures = 0;
            tiger_reposition_timer.SetTimeout(0);
            tiger.wander_state = 0;
            set_mode(AI_STATE_COMBAT, 0x21);
        } else if (idx == 1) {
            tiger_reset_b58();
            set_mode(AI_STATE_COMBAT, 0x20);
        } else if (idx == 2 && !tiger.weapon_ready) {
            tiger_reset_b58();
            set_mode(AI_STATE_COMBAT, 0x20);
        } else {
            combat_approach(t);
        }
        break;
    case 1:
        if (tiger.pending_submode == 0x21)
            goto wander;
        if (tiger.pending_submode == 0xe)
            set_mode(AI_STATE_COMBAT, 0xe);
        break;
    }
    if (tiger_target == -1)
        tiger_target = target;
    target = tiger_target;
    target = tiger_target;
    owner->combat.target = target;
    t = living_from_handle(target);
    if (t == NULL || (t->flags & CF_HIDDEN)) {
        go_idle();
        return;
    }
    bool done = false;
    switch (submode) {
    case 14:
        done = tiger_stage_move_update();
        break;
    case 15:
        combat_melee_attack(t);
        break;
    case 22:
    case 23:
        combat_close_in(t);
        break;
    case 16:
        combat_ranged_attack(t);
        break;
    case 0x1f:
        tiger_combat_update(t);
        break;
    case 0x1e:
        tiger_approach_target(t);
        break;
    case 0x21:
        tiger_wander_update();
        break;
    case 0x20:
        tiger_reposition_update(t);
        break;
    case -1:
        return;
    }
    if (submode == 14 && done == true) {
        idx = tiger_current_move_index();
        if (idx == 0 || idx == 2) {
            if (!combat_approach(t))
                go_idle();
            return;
        }
        if (idx == 1) {
            tiger_reset_b54();
            set_mode(AI_STATE_COMBAT, 0x1e);
            return;
        }
        go_idle();
    }
}

// 0x49c130  Confidence: likely. As serpent_anim_remap_in. D19401: the weight stored in each branch (also
// in both arms of the ready test) gives the binary's zero in EBP, ready in c's slot read in the loop, the
// unfolded 3 / 0 stores and both arrays addressed from the one set pointer; left: the three loop registers
// rotated (ECX / EDX / ESI in ours, EDX / ESI / ECX in the binary) and push esi one instruction late.
void tiger_anim_remap_in(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    Character *mount = c->get_mount();
    if (mount == NULL)
        return;
    int ready = 0;
    Character *r = mount->get_rider();
    if (r)
        ready = r->flags & CF_COMBAT_READY;
    tiger_anim_remap_count = set->count;
    for (int i = 0; i < set->count; i++) {
        int a = set->anim[i];
        if (a == mount->anim_stand) {
            if (ready) {
                tiger.remap_slot[i] = 3;
                tiger.remap_weight[i] = set->weight[i];
            } else {
                tiger.remap_slot[i] = 0;
                tiger.remap_weight[i] = set->weight[i];
            }
        } else if (a == mount->anim_walk) {
            tiger.remap_slot[i] = 4;
            tiger.remap_weight[i] = set->weight[i];
        } else if (a == mount->anim_run) {
            tiger.remap_slot[i] = 5;
            tiger.remap_weight[i] = set->weight[i];
        }
    }
}

// 0x49c1e0  Confidence: exact.
void tiger_anim_remap_out(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    Character *mount = c->get_mount();
    if (mount == NULL)
        return;
    for (int i = 0; i < tiger_anim_remap_count; i++) {
        set->anim[i] = c->loco_anims[tiger.remap_slot[i]];
        set->weight[i] = tiger.remap_weight[i];
    }
}

// 0x49c240  Confidence: exact (the move tests inside "if (emerge_move >= 0)", D10305; the case order 4, 3, 2, 1, 0 from the bodies). Sub-mode 0xe: the move
// that puts the next weapon in hand (type 10, id 0xb7 / 0xb5 / 0xb4 / 0xb3 / 0xb2 by spear uses left);
// at its trigger or end the stage's ready animations; true when it ended.
bool ai_state::tiger_stage_move_update()
{
    int id;

    ai_move_flags |= 4;
    switch (tiger_spear_uses_left) {
    case 4:
        id = 0xb2;
        break;
    case 3:
        id = 0xb3;
        break;
    case 2:
        id = 0xb4;
        break;
    case 1:
        id = 0xb5;
        break;
    case 0:
        id = 0xb7;
        break;
    default:
        return true;
    }
    if (!disable_timer.valid()) {
        if (owner->find_move(10, id) == NULL)
            return false;
        disable_timer.SetTimeout(0);
        move_info *m = owner->find_move(10, id);
        emerge_move = -1;
        if (m) {
            emerge_move = owner->play_move((int)m, 1.0f, false, false, false);
            if (emerge_move == -1)
                return false;
            if (owner->flags & CF_COMBAT_READY)
                owner->stance_blend = 0;
            else
                owner->stance_blend = 1.0f;
        }
        flags = (flags & 0x7ffffffd) | 2;
    }
    if (emerge_move >= 0) {
        if (owner->move_finished(emerge_move)) {
            if (owner->flags & CF_COMBAT_READY) {
                owner->prop_tag = 0;
                owner->attachments[4 - tiger_spear_uses_left].slot = 0;
                if (tiger_stage == 1)
                    tiger_set_stage1_anims(owner);
                else
                    tiger_set_stage2_anims(owner);
                tiger.weapon_ready = true;
            }
            flags &= ~2;
            disable_timer.invalidate();
            return true;
        }
        if (owner->move_trigger_fired(10, 1)) {
            if (tiger_stage == 1)
                tiger_set_stage1_anims(owner);
            else
                tiger_set_stage2_anims(owner);
            tiger.weapon_ready = true;
            if (owner->get_current_move()->id == id) {
                owner->prop_tag = 0;
                owner->attachments[4 - tiger_spear_uses_left].slot = 0;
            }
        }
        if (owner->move_progress(emerge_move) >= 0.5f && !(flags & 0x80000000)) {
            flags |= 0x80000000;
            owner->flags |= CF_COMBAT_READY;
            if (owner->flags & CF_COMBAT_READY) {
                owner->stance_blend = 0;
                return false;
            }
            owner->stance_blend = 1.0f;
        }
    }
    return false;
}

// 0x49c4a0  Confidence: exact (VC6 byte match). boss_set_foot_anims: stage 3 on foot, the axe in hand.
void tiger_set_foot_anims(Character *c)
{
    int anim;

    tiger_has_rider = false;
    anim = charinfo_find_anim(c->info, "ready");
    if (anim != -1) {
        c->anim_ready = anim;
        c->anim_stand = anim;
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
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
    if (tiger_axe) {
        item_info *old1, *old2;
        character_equip_item(&old1, &old2, c, 5, tiger_axe);
        tiger_axe = NULL;
        if (old1)
            item_free(old1);
        if (old2)
            item_free(old2);
    }
    c->attachments[4].slot = 0;
    c->attachments[0].slot = 0x11;
    c->attachments[1].slot = 0x11;
    c->attachments[2].slot = 0x11;
    c->attachments[3].slot = 0x11;
    c->num_attachments = 5;
    tiger_spear_uses_left = 0;
    tiger_stage = 3;
    tiger_target_stage = 3;
}

// 0x49c610  Confidence: exact (VC6 byte match) (the store order). boss_dismount_dispatch for the tiger, phoenix and
// serpent riders: rider_unlink_mount written out.
void boss_dismount(Character *c)
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

// folded (0x49c610, boss_dismount)  Confidence: likely. boss_dismount_dispatch case 0x57: the name the dispatcher
// calls for the tiger (D7603 (2), D8902). In Sum.exe it is the copy above, the one the linker kept of the three
// identical rider dismounts; boss_dismount keeps its marker, so this is a second copy of the same body.
void tiger_dismount(Character *c)
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

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_script_hook_stub case 0x57 (boss_ai.h: folded).
void tiger_script_hook(Character *c)
{
}

// 0x49c670  Confidence: exact. After a spear move (move 0): one use less, the weapon to be drawn again.
void tiger_move_done_hook(Character *c)
{
    if (tiger_current_move_index() == 0) {
        tiger_spear_uses_left--;
        tiger.weapon_ready = false;
    }
}

// 0x49c690  Confidence: exact (VC6 byte match) (cases 1 and 2-5 have separate, identical bodies). boss_move_in_range_dispatch:
// the spear reaches spell 0x37's distance, the other moves twice the attack radius.
int tiger_move_in_range(Character *boss, Character *target)
{
    float d = boss->distance_between_edges(target, true, -1);
    float reach;
    switch (tiger_current_move_index()) {
    case 0:
        reach = spells[0x37].distance;
        break;
    case 1:
        reach = boss->info->attack_radius * 2.0f;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        reach = boss->info->attack_radius * 2.0f;
        break;
    default:
        reach = 0.0f;
        break;
    }
    if (d < reach)
        return 1;
    return 0;
}

// 0x49c720  Confidence: exact. boss_get_attack_range: the spear's attack radius in stage 1.
bool tiger_get_attack_range(Character *c, float *out)
{
    if (tiger_stage == 1 && tiger_current_move_index() == 0) {
        *out = tiger_weapon->type->attack_radius;
        return true;
    }
    return false;
}

// 0x49c750  Confidence: exact. The cutscene / level script reset: the ride animations and the next
// spear tag shown again.
void tiger_restore_ride_anims(Character *rider)
{
    tiger_set_ride_stand_anims(rider);
    rider->attachments[4 - tiger_spear_uses_left].slot = 0x11;
}

// 0x49c780  Confidence: exact. boss_move_trigger_dispatch: trigger 7 of the charge once it struck.
bool tiger_move_trigger(Character *c)
{
    if (c->info->species == SPECIES_TIGER_RIDER && tiger_current_move_index() == 1
        && tiger_move_trigger_pending == true) {
        tiger_move_trigger_pending = false;
        c->move_trigger_mark(7);
        return true;
    }
    return false;
}

// 0x49c7c0  Confidence: exact. cmd_tiger_solo and the level scripts: on foot in stage 3, combat on.
void tiger_set_solo(Character *rider)
{
    rider_unlink_mount(rider);
    rider->flags |= CF_COMBAT_READY;
    tiger_setup_anims(rider);
    rider->process_anim(0.0f);
    rider->find_ground();
    rider->attachments[4].slot = 0;
    rider->attachments[0].slot = 0x11;
    rider->attachments[1].slot = 0x11;
    rider->attachments[2].slot = 0x11;
    rider->attachments[3].slot = 0x11;
    rider->num_attachments = 5;
    tiger_stage = 3;
    tiger_target_stage = 3;
    tiger_combat = true;
}
