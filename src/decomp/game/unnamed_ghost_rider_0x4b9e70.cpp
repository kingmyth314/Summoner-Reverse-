// unnamed_ghost_rider_0x4b9e70.cpp - Sum.exe v1.40, code 0x4b9e70-0x4bbe77: the Ghost Rider boss (species
// 0x5a) and its minions. File name NOT known: a placeholder (src/decomp/README.md). No assert path, no
// strings.tbl block; cut by the static initialiser run (entries 309-313, 0x5884d8-0x5884e8) and the data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_ccmd_ghost_stage (0x4b9e70) to ghost_minions_deactivate (0x4bbdd0-0x4bbe77);
// the decal unit opens with its initialiser 0x4bbe80 (entry 314).
// Data: .data 0x59d9ec-0x59daeb (the four command names / help texts, the combat message, "jadetemple",
// "ghost_rider_boss.tbl"); "Liangshan", "Jadetemple", "masad", "stand" / "walk" / "run",
// "tripped_liedown", "stage1_*" and "$sit" are other units' literals (pooled). .bss 0x24802e0-0x2480947:
// it starts with ghost_move_3_2 (0x24802e0, right after Pyrul's last pointer) and ends with ghost_loaded
// (0x2480944); docs/sum-exe/source-map.md's cut gives 0x2480300, the first command object.
// The minion arrays are three (servants, mages, undead): ghost_revive_minions has three separate loops
// and ghost_choose_move_target picks one array per summon spell.

#include <string.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"

#define AIS_CLOSE_COMBAT  0x0f        // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_RANGED_COMBAT 0x10
#define AIS_GET_CLOSE     0x16
#define AIS_GET_FAR       0x17
#define AIS_SAFETY        0x20

#define SPELL_GHOST_SUMMON_BONE_SERVANT 0x51
#define SPELL_GHOST_SUMMON_BONE_MAGE    0x52
#define SPELL_GHOST_SUMMON_UNDEAD       0x53

#define SPECIES_BONE_SERVANT 0x43     // names ours, from the summon spells 0x51-0x53
#define SPECIES_BONE_MAGE    0x45
#define SPECIES_UNDEAD       0x9c

// ---- .bss (address order) -----------------------------------------------------------------------------
boss_attack *ghost_move_3_2;          // 0x24802e0
boss_attack *ghost_move_1_2;          // 0x24802e4
boss_attack *ghost_move_2_2;          // 0x24802e8
Character *ghost_mount;               // 0x24802ec
boss_attack *ghost_move_3_0;          // 0x24802f0
boss_attack *ghost_move_3_1;          // 0x24802f4
boss_attack *ghost_move_2_1;          // 0x24802f8
boss_attack *ghost_move_2_0;          // 0x248030c
ghost_boss ghost;                     // 0x2480310-0x248090f (D6511: the fields' old names are macros in boss_ai.h)
boss_attack *ghost_move_1_0;          // 0x248091c
boss_attack *ghost_move_1_1;          // 0x2480920
bool ghost_loaded;                    // 0x2480944

// 0x4b9e70  Confidence: exact. static_init_ccmd_ghost_stage: "jmp 0x4b9e80" (initialiser entry 309).
// 0x4b9e80  Confidence: exact. init_ccmd_ghost_stage (console_command constructor).
console_command ccmd_ghost_stage("ghost_stage", "Set Ghost rider's current combat stage", cmd_ghost_stage);   // 0x2480928

// 0x4b9ea0  Confidence: exact. Like cmd_pyrul_stage, but without the error message.
void cmd_ghost_stage()
{
    process_command(ARG_INT);
    if (arg_type & ARG_INT) {
        if (command_argument_int >= 0 && command_argument_int <= 3)
            ghost_target_stage = command_argument_int;
    }
}

// 0x4b9ed0  Confidence: exact. static_init_ccmd_ghost_unlink: "jmp 0x4b9ee0" (entry 310).
// 0x4b9ee0  Confidence: exact. init_ccmd_ghost_unlink (console_command constructor).
console_command ccmd_ghost_unlink("ghost_unlink", "Unlinks ghost from mount", cmd_ghost_unlink);   // 0x2480910

// 0x4b9f00  Confidence: exact (VC6 byte match) (store order). The first Ghost Rider of the living list (ghost_find_rider
// returns the last one) is unlinked from its mount; the mount keeps existing (rider_unlink_mount, the
// solo path, deletes it).
void cmd_ghost_unlink()
{
    Character *c;
    Character *rider = NULL;
    Character *mount;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->info->species == SPECIES_GHOST_RIDER) {
            rider = c;
            break;
        }
    }
    if (rider) {
        mount = rider->get_mount();
        if (mount) {
            mount->mount_flags = 0;
            mount->mount_link = -1;
            mount->ai.target = -1;
            mount->flags &= ~0x20;
            rider->mount_flags = 0;
            rider->mount_link = -1;
            rider->flags |= CF_COMBAT_READY;
        }
    }
}

// 0x4b9f90  Confidence: exact. static_init_ccmd_ghost_combat: "jmp 0x4b9fa0" (entry 311).
// 0x4b9fa0  Confidence: exact. init_ccmd_ghost_combat (console_command constructor).
console_command ccmd_ghost_combat("ghost_combat", "Puts ghost rider in combat state", cmd_ghost_combat);   // 0x2480300

// 0x4b9fc0  Confidence: exact.
void cmd_ghost_combat()
{
    ghost_combat_enabled = true;
    print_to_console("Setting ghost's ai state to combat\n", 0);
}

// 0x4b9fe0  Confidence: exact. The last living entity of species 0x5a, NULL when none.
Character *ghost_find_rider()
{
    Character *c;
    Character *rider = NULL;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->info->species == SPECIES_GHOST_RIDER)
            rider = c;
    }
    return rider;
}

// 0x4ba020  Confidence: exact. static_init_ccmd_ghost_solo: "jmp 0x4ba030" (entry 312).
// 0x4ba030  Confidence: exact. init_ccmd_ghost_solo (console_command constructor; the help text ends
// with a newline in the binary).
console_command ccmd_ghost_solo("ghost_solo", "Set ghost without horse\n", cmd_ghost_solo);   // 0x2480938

// 0x4ba050  Confidence: exact.
void cmd_ghost_solo()
{
    Character *rider = ghost_find_rider();

    if (rider)
        ghost_set_solo(rider);
}

// 0x4ba070  Confidence: exact. static_init_ghost_timers: "jmp 0x4ba080" (entry 313).
// 0x4ba080  Confidence: likely. init_ghost_timers: the five threat timers, then ghost_target_timer,
// ghost_think_timer, ghost_wander_timer, ghost_fear_timer and ghost_summon_timer (the compiler's
// initialiser for the definitions above; it comes after the command objects', so the timers are defined
// after them in the original, unlike the order of the .bss).

// 0x4ba0d0  Confidence: likely (the ray is a local whose inline constructor clears +0x30 at entry; the
// start point is a vector3 local whose y is reused for the hit height, match31: frame now 0x94). Casts
// a 40-long ray down from 20 above the point against the level objects of the layer (the layer list, or
// every object with that layer in one of its first two layer bytes) and moves the point to the highest
// hit above pos->y - 20.
void ghost_find_floor_below(Character *unused, int layer, vector3 *pos)
{
    lobj_ray ray;
    lobj_hit hit;
    layer_lobj_list *list;
    float best;
    float y;
    bool found;
    int count;
    int i;
    int index;

    best = pos->y - 20.0f;
    found = false;
    hit.num_hits = 0;
    list = level_layer_ground[layer];
    if (list)
        count = list->count;
    else
        count = num_lobjs;
    for (i = 0; i < count; i++) {
        if (list) {
            index = list->indices[i];
        } else {
            index = i;
            if (lobjs[i].get_layer_n(0) != layer && lobjs[i].get_layer_n(1) != layer)
                continue;
        }
        vector3 start(pos->x, pos->y + 20.0f, pos->z);
        ray.start = start;
        ray.dir.x = 0.0f;
        ray.dir.y = -40.0f;
        ray.dir.z = 0.0f;
        ray._unk18 = 0.0f;
        ray.flags = 0;
        ray.require_flags = 0;
        ray.exclude_flags = 0;
        lobj_raycast(index, &ray, &hit, true);
        if (hit.num_hits > 0) {
            y = hit.t * ray.dir.y + start.y;
            if (y > best) {
                best = y;
                found = true;
            }
        }
    }
    if (found)
        pos->y = best;
}

// 0x4ba260  Confidence: exact (VC6 byte match, match31; was likely) (the weapon swap and the three minion loops; the minion positions are one
// forward vector ahead of the boss). Runs once per level (ghost_loaded). Liangshan and Jadetemple start at
// stage 3 on foot, the other levels at stage 1 mounted. Masad has no minions; in jadetemple the mage and
// undead slots repeat the two servants.
void ai_state::ghost_rider_boss_load()
{
    int i;
    int tag;
    int info;
    item_info *old_a;
    item_info *old_b;
    Character *c;
    vector3 pos;

    ghost_dismount_requested = false;
    ghost_dismount_done = false;
    if (ghost_loaded == true)
        return;
    ghost_loaded = true;
    if (_stricmp(level_filename, "Liangshan") == 0 || _stricmp(level_filename, "Jadetemple") == 0) {
        ghost_stage = 3;
        ghost_target_stage = 3;
        ghost_setup_anims_unmounted(owner);
        ghost_mounted = false;
    } else {
        ghost_stage = 1;
        ghost_target_stage = 0;
        ghost_set_stage1_anims(owner);
        ghost_mounted = true;
    }
    ghost_target = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        ghost_rider_threat_table[i].handle = -1;
    ghost_num_offense_points = 0;
    for (i = 0; i < 10; i++) {
        const char *name = offense_point_name(i);
        ca_character *ch = owner->instance->character;
        tag = ch->find_mesh_tag(name);
        if (tag != -1) {
            ghost_offense_points[ghost_num_offense_points].tag = tag;
            ghost_num_offense_points++;
        }
    }
    boss_ai_parse_table("ghost_rider_boss.tbl", 3, ghost_attack_table, ghost_num_offense_points);
    ghost_init_move_ptrs();
    ghost_target_timer.SetTimeout(0);
    ghost_summon_timer.SetTimeout(0);
    ghost_wander_timer.invalidate();
    ghost_fear_timer.SetTimeout(0);
    ghost_timer_set_random(5000, 10000);
    if (ghost_stage == 1)
        ghost_seat_rider();

    i = owner->info->default_weapons[0];
    character_equip_item(&old_a, &old_b, owner, 5, item_create_weapon(i));
    if (old_a)
        item_free(old_a);
    if (old_b)
        item_free(old_b);
    owner->attachments[0].item_type = i;
    owner->attachments[0].slot = 0x11;
    owner->num_attachments = 1;
    ghost_minions_raised = 0;

    if (_stricmp(level_filename, "masad") == 0)
        return;
    info = charinfo_find_by_species(SPECIES_BONE_SERVANT);
    for (i = 0; i < 2; i++) {
        pos = owner->pos + owner->orient.fvec;
        c = living_entity_create(5, info, &pos, &owner->orient, true, owner->layer, 0, 0, false);
        if (c) {
            ghost_bone_servants[i] = c->handle;
            c->set_ai_active(0);
            c->flags |= CF_HIDDEN;
        }
    }
    if (_stricmp(level_filename, "jadetemple") == 0) {
        for (i = 0; i < 2; i++) {
            ghost_bone_mages[i] = ghost_bone_servants[i];
            ghost_undead[i] = ghost_bone_servants[i];
        }
    } else {
        info = charinfo_find_by_species(SPECIES_BONE_MAGE);
        for (i = 0; i < 2; i++) {
            pos = owner->pos + owner->orient.fvec;
            c = living_entity_create(5, info, &pos, &owner->orient, true, owner->layer, 0, 0, false);
            if (c) {
                ghost_bone_mages[i] = c->handle;
                c->set_ai_active(0);
                c->flags |= CF_HIDDEN;
            }
        }
        info = charinfo_find_by_species(SPECIES_UNDEAD);
        for (i = 0; i < 2; i++) {
            pos = owner->pos + owner->orient.fvec;
            c = living_entity_create(5, info, &pos, &owner->orient, true, owner->layer, 0, 0, false);
            if (c) {
                ghost_undead[i] = c->handle;
                c->set_ai_active(0);
                c->flags |= CF_HIDDEN;
            }
        }
    }
    ghost_mount = owner->get_mount();
}

// 0x4ba690  Confidence: exact (max - min is stored into max's slot for fimul).
void ghost_timer_set_random(int min_ms, int max_ms)
{
    ghost_think_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4ba6c0  Confidence: exact. Mounted locomotion: the "stage1_*" anims into the normal, ready and hurt
// slots; no lie-down anim while mounted (kept in ghost_saved_trip_anim).
void ghost_set_stage1_anims(Character *c)
{
    int anim;

    boss_ensure_mount(c);
    ghost_saved_trip_anim = c->anim_tripped_liedown;
    c->anim_tripped_liedown = -1;
    anim = charinfo_find_anim(c->info, "stage1_stand");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "stage1_walk");
    if (anim != -1) {
        c->anim_norm_walk = anim;
        c->anim_walk = anim;
        c->anim_norm_walk_hurt = anim;
        c->anim_walk_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "stage1_run");
    if (anim != -1) {
        c->anim_norm_run = anim;
        c->anim_run = anim;
        c->anim_norm_run_hurt = anim;
        c->anim_run_hurt = anim;
    }
}

// 0x4ba780  Confidence: exact. ghost_setup_anims already clears the attachment slot; this clears it again.
void ghost_setup_anims_unmounted(Character *c)
{
    ghost_setup_anims(c);
    c->attachments[0].slot = 0;
}

// 0x4ba7a0  Confidence: exact. On-foot locomotion, the lie-down anim back, the mount released.
void ghost_setup_anims(Character *c)
{
    int anim;

    anim = charinfo_find_anim(c->info, "stand");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
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
    c->attachments[0].slot = 0;
}

// 0x4ba870  Confidence: exact (store order as in the binary).
void ghost_init_move_ptrs()
{
    ghost_move_1_0 = &ghost_attack_table[0][0];
    ghost_move_1_1 = &ghost_attack_table[0][1];
    ghost_move_1_2 = &ghost_attack_table[0][2];
    ghost_move_2_0 = &ghost_attack_table[1][0];
    ghost_move_2_1 = &ghost_attack_table[1][1];
    ghost_move_2_2 = &ghost_attack_table[1][2];
    ghost_move_3_0 = &ghost_attack_table[2][0];
    ghost_move_3_1 = &ghost_attack_table[2][1];
    ghost_move_3_2 = &ghost_attack_table[2][2];
}

// 0x4ba8d0  Confidence: exact.
void ghost_unload()
{
    ghost_loaded = false;
}

// folded (0x4f13e0, empty_function)  Confidence: likely. boss_script_hook_stub case 0x5a (boss_ai.h: folded).
void ghost_script_hook(Character *c)
{
}

// 0x4ba8e0  Confidence: exact (VC6 byte match) (the matrix copies: VC6 expands matrix::operator= only partly here and
// calls vector3::operator= / the copy constructor for the last row). Rider sound at the load-time mount,
// then, while mounted, the rider is put on the mount's "$sit" bone (or at the mount's origin) and the
// mount gets the rider's target. The animation time is read and written back unchanged (0x4ba931).
bool ai_state::ghost_seat_rider()
{
    Character *mount;
    int bone;
    float t;
    matrix orient;
    vector3 pos;

    if (ghost_mount)
        boss_rider_random_sound(&ghost_mount->pos);
    if (ghost_mounted) {
        mount = owner->get_mount();
        if (mount) {
            mount->ai.target = target;
            mount->instance->get_set_time(&t, false);
            mount->instance->get_set_time(&t, true);
            bone = mount->instance->character->find_mesh_tag("$sit");
            if (bone != -1) {
                mount->instance->get_bone_world_transform(bone, &mount->orient, &mount->pos, &orient, &pos);
                owner->pos = pos;
                owner->orient = orient;
                return true;
            }
            owner->pos = mount->pos;
            owner->orient = mount->orient;
            return true;
        }
    }
    return false;
}

// 0x4baa60  Confidence: exact. As pyrul_ai_idle with the ghost's flag and move.
void ai_state::ghost_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (ghost_combat_enabled == true) {
        ghost_cur_move = NULL;
        enter_combat();
    }
}

// 0x4baaa0  Confidence: exact. Sub-mode AIS_SAFETY: wander along the path; when it fails or the 5 s are
// up, back to closing in (AIS_GET_CLOSE).
void ai_state::ghost_wander_update()
{
    if (!follow_path(0) || ghost_wander_timer.elapsed()) {
        ghost_timer_set_random(5000, 10000);
        set_mode(AI_STATE_COMBAT, AIS_GET_CLOSE);
    }
}

// 0x4baae0  Confidence: exact (8 tries, `while (tries--)` from 8; the horizontal distance is a vector
// difference with y cleared and len_sq, D7700). A navigable point 10 away in a random direction, more
// than 7 away horizontally, with a path; the mount travels.
bool ai_state::ghost_wander_start()
{
    int tries = 8;
    Character *mount;

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
                mount = owner->get_mount();
                if (mount) {
                    mount->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
                    ghost_wander_timer.SetTimeout(5000);
                    return true;
                }
            }
        }
    }
    ghost_think_timer.SetTimeout(500);
    return false;
}

// 0x4bac20  Confidence: exact (VC6 byte match, match31; was unsure) (the three stage blocks; the tails are shared). Returns 0 (no target), 1
// (stage 1, more than one attacker: wander, ghost_pending_submode = AIS_SAFETY), 3 (new move) or 5 (the
// current move goes on). The summon move needs fewer than 2 minions out; paralysing fear goes to a
// target that is not paralysed (filter flag 1).
int ai_state::ghost_choose_attack()
{
    boss_target list[BOSS_MAX_THREATS];
    Character *target;
    bool moving;
    bool busy;
    bool timer_up;
    int count;
    int handle;

    target = living_from_handle(ghost_target);
    if (target && (target->flags & CF_HIDDEN))
        target = NULL;
    moving = owner->get_current_move() != NULL;
    busy = owner->combat.is_busy();
    if (!moving && busy) {
        owner->combat.state = COMBAT_IDLE;
        busy = false;
    }
    if (target == NULL) {
        ghost_2480818 = 0;
        ghost_cur_move = NULL;
    }
    if (busy == true)
        return ghost_cur_move ? 5 : 0;
    if (ghost_cur_move)
        return 5;

    timer_up = ghost_target_timer.elapsed();
    ghost_update_attack_points(owner);
    count = boss_target_list_build(owner, list, ghost_rider_threat_table);

    if (ghost_stage == 1) {
        if (timer_up || !target) {
            handle = boss_target_list_pick(list, count, ghost_rider_threat_table, ghost_target, false, 0);
            ghost_target = handle;
            target = living_from_handle(handle);
            if (handle == -1)
                goto none;
            ghost_timer2_set_random(20000, 30000);
        }
        if (ghost_think_timer.elapsed()) {
            if (count_attackers() > 1) {
                ghost_pending_submode = AIS_SAFETY;
                return 1;
            }
            ghost_timer_set_random(5000, 10000);
        }
        if (ghost_minions_raised < 2 && ghost_summon_timer.elapsed()) {
            ghost_choose_move_target(owner);
            if (ghost_cur_move) {
                ghost_summon_timer.SetRandomTimeout(20000, 40000);
                return 3;
            }
        }
        if (ghost_fear_timer.elapsed()) {
            handle = ghost_target;
            if (target && target->has_status(STATUS_PARALYZE))
                handle = boss_target_list_pick(list, count, ghost_rider_threat_table, ghost_target, true, 1);
            if (handle != -1) {
                ghost_fear_timer.SetTimeout(45000);
                ghost_target = handle;
                ghost_cur_move = ghost_move_1_1;
                return 3;
            }
        }
        ghost_cur_move = ghost_move_1_2;
        return 3;
    } else if (ghost_stage == 2) {
        if (timer_up || !target) {
            handle = boss_target_list_pick(list, count, ghost_rider_threat_table, ghost_target, false, 0);
            ghost_target = handle;
            if (handle == -1)
                goto none;
            ghost_timer2_set_random(20000, 30000);
        }
        if (ghost_minions_raised < 2 && ghost_summon_timer.elapsed()) {
            ghost_choose_move_target(owner);
            if (ghost_cur_move) {
                ghost_summon_timer.SetRandomTimeout(20000, 40000);
                return 3;
            }
        }
        if (frand() < 0.7f) {
            ghost_cur_move = ghost_move_2_2;
            return 3;
        }
        ghost_cur_move = ghost_move_2_1;
        return 3;
    } else {
        if (timer_up || !target) {
            handle = boss_target_list_pick(list, count, ghost_rider_threat_table, ghost_target, false, 0);
            ghost_target = handle;
            if (handle != -1)
                ghost_timer2_set_random(20000, 30000);
            else
                goto none;
        }
        if (frand() > 0.7f && ghost_fear_timer.elapsed()) {
            handle = ghost_target;
            if (target && target->has_status(STATUS_PARALYZE))
                handle = boss_target_list_pick(list, count, ghost_rider_threat_table, ghost_target, true, 1);
            if (handle != -1) {
                ghost_fear_timer.SetTimeout(30000);
                ghost_target = handle;
                ghost_cur_move = ghost_move_3_1;
                return 3;
            }
        }
        ghost_cur_move = ghost_move_3_2;
        return 3;
    }
none:
    return 0;
}

// 0x4bb050  Confidence: exact.
void ghost_timer2_set_random(int min_ms, int max_ms)
{
    ghost_target_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4bb080  Confidence: exact (VC6 byte match). Same body as pyrul_update_attack_points on the ghost's points.
void ghost_update_attack_points(Character *c)
{
    int i;
    matrix orient;
    vector3 pos;
    boss_offense_point *pt;

    for (i = 0; i < ghost_num_offense_points; i++) {
        pt = &ghost_offense_points[i];
        c->instance->get_bone_world_transform(pt->tag, &c->orient, &c->pos, &orient, &pos);
        pt->dir = orient.uvec;
        pt->pos = pos + c->centre_offset;
    }
}

// 0x4bb120  Confidence: exact (VC6 byte match, match31; was likely). The summon move of the stage (stage 3: servants 33 %, mages 33 %, undead
// 34 %) and the first minion of its pair that is inactive and not dead; the summon point is halfway to the
// target (cur_character without one), or, when that is nearer than 4 to the target, a navigable point 4-8
// from the boss (10 tries), dropped onto the floor.
void ghost_choose_move_target(Character *boss)
{
    boss_attack *move;
    int *slots;
    int i;
    float f;
    Character *c;
    Character *t;

    ghost_summon_minion = -1;
    ghost_cur_move = NULL;
    if (ghost_stage == 1) {
        move = ghost_move_1_0;
        slots = ghost_bone_servants;
    } else if (ghost_stage == 2) {
        move = ghost_move_2_0;
        slots = ghost_bone_mages;
    } else {
        f = frand();
        if (f < 0.33f) {
            move = ghost_move_1_0;
            slots = ghost_bone_servants;
        } else if (f < 0.66f) {
            move = ghost_move_2_0;
            slots = ghost_bone_mages;
        } else {
            move = ghost_move_3_0;
            slots = ghost_undead;
        }
    }
    for (i = 0; i < 2; i++) {
        c = living_from_handle(slots[i]);
        if (!c)
            return;
        if (!(c->obj_flags & 1) && !(c->flags & CF_DEAD)) {
            ghost_summon_minion = slots[i];
            ghost_cur_move = move;
            break;
        }
    }
    if (ghost_summon_minion == -1)
        return;
    ghost_summon_pos = boss->pos;
    t = living_from_handle(ghost_target);
    if (!t) {
        t = cur_character;
        if (!t)
            return;
    }
    ghost_summon_pos = (t->pos + boss->pos) * 0.5f;
    if (distance(&ghost_summon_pos, &t->pos) < 4.0f) {
        float r;
        vector3 p;
        for (i = 0; i < 10; i++) {
            r = frand_range(4.0f, 8.0f);
            p.random_direction();
            p.y = 0.0f;
            p *= r;
            p += boss->pos;
            if (pf_point_is_navigable(&p, t->info->size, boss->layer) == true)
                break;
        }
        if (i == 10)
            return;
        ghost_find_floor_below(t, boss->layer, &p);
        ghost_summon_pos = p;
    }
}

// 0x4bb380  Confidence: exact (the sub-mode switch: case -1 has its own slot, 0x4bb498 byte 0; bodies
// laid out 15, 22/23, 32, 16; the new target handle is a local, D7700).
void ai_state::ghost_ai_frame()
{
    Character *t;
    int result;
    int h;

    t = living_from_handle(target);
    if (t && (t->flags & CF_HIDDEN))
        t = NULL;
    result = ghost_choose_attack();
    switch (result) {
    case 0:
        go_idle();
        return;
    case 1:
        if (ghost_pending_submode == AIS_SAFETY) {
            if (!ghost_wander_start())
                return;
            ghost_timer_set_random(5000, 10000);
            set_mode(AI_STATE_COMBAT, AIS_SAFETY);
        }
        break;
    }
    h = ghost_target;
    if (h != target) {
        target = h;
        owner->combat.target = h;
        flags |= 0x10;
        t = living_from_handle(target);
    }
    if (!t) {
        go_idle();
        return;
    }
    if (result == 3)
        combat_approach(t);
    switch (submode) {
    case -1:
        return;
    case AIS_CLOSE_COMBAT:
        combat_melee_attack(t);
        break;
    case AIS_GET_CLOSE:
    case AIS_GET_FAR:
        combat_close_in(t);
        break;
    case AIS_SAFETY:
        ghost_wander_update();
        break;
    case AIS_RANGED_COMBAT:
        combat_ranged_attack(t);
        break;
    }
}

// 0x4bb4c0  Confidence: exact (VC6 byte match) (forced-stage switch bodies laid out default, 3, 2, 1). The stage 2 step
// waits for the level script: the first time below 66 % it sets ghost_dismount_requested, and switches
// only once ghost_dismount_done is set (the Ghost-Dismount cutscene end).
void ai_state::ghost_update_stage()
{
    float frac;

    if (ghost_target_stage != 0) {
        if (ghost_stage == ghost_target_stage)
            return;
        switch (ghost_target_stage) {
        default:
            ghost_target_stage = 0;
            return;
        case 3:
            ghost_setup_anims_unmounted(owner);
            ghost_stage = 3;
            return;
        case 2:
            ghost_setup_anims(owner);
            ghost_stage = 2;
            return;
        case 1:
            ghost_set_stage1_anims(owner);
            ghost_stage = 1;
            return;
        }
    }
    frac = owner->hp / (float)owner->get_max_stat(0);
    if (frac < 0.3f) {
        if (ghost_stage != 3) {
            ghost_setup_anims_unmounted(owner);
            ghost_stage = 3;
        }
    } else if (frac < 0.66f) {
        if (ghost_stage != 2) {
            if (!ghost_dismount_requested) {
                ghost_dismount_requested = true;
                ghost_dismount_done = false;
            } else if (ghost_dismount_done) {
                ghost_setup_anims(owner);
                ghost_stage = 2;
            }
        }
    } else {
        if (ghost_stage != 1) {
            ghost_set_stage1_anims(owner);
            ghost_stage = 1;
        }
    }
}

// 0x4bb610  Confidence: likely. character_boss_anim_remap_in for the rider: the mount's stand (ready when
// the mount's rider is in the ready stance), walk and run anims of the blend become locomotion indices 0 /
// 3, 4 and 5, with their weights; other anims are left out (their kind and weight slots keep old values).
void ghost_anim_remap_in(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    Character *mount;
    Character *rider;
    int ready;
    int i;

    mount = c->get_mount();
    if (!mount)
        return;
    ready = 0;
    rider = mount->get_rider();
    if (rider)
        ready = rider->flags & CF_COMBAT_READY;
    ghost_remap_count = set->count;
    for (i = 0; i < set->count; i++) {
        if (set->anim[i] == mount->anim_stand) {
            if (ready)
                ghost_remap_kind[i] = 3;
            else
                ghost_remap_kind[i] = 0;
        } else if (set->anim[i] == mount->anim_walk) {
            ghost_remap_kind[i] = 4;
        } else if (set->anim[i] == mount->anim_run) {
            ghost_remap_kind[i] = 5;
        } else {
            continue;
        }
        ghost_remap_weight[i] = set->weight[i];
    }
}

// 0x4bb6c0  Confidence: exact (VC6 byte match). Writes the rider's own anims of the remembered kinds back into the blend
// (the count is not restored).
void ghost_anim_remap_out(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    int i;

    if (!c->get_mount())
        return;
    for (i = 0; i < ghost_remap_count; i++) {
        set->anim[i] = c->loco_anims[ghost_remap_kind[i]];
        set->weight[i] = ghost_remap_weight[i];
    }
}

// 0x4bb720  Confidence: exact. boss_set_foot_anims case 0x5a: on foot, stage 2 forced.
void ghost_set_foot_anims(Character *c)
{
    int anim;

    ghost_mounted = false;
    anim = charinfo_find_anim(c->info, "stand");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
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
    ghost_stage = 2;
    ghost_target_stage = 2;
}

// 0x4bb810  Confidence: exact (VC6 byte match) (store order). boss_dismount_dispatch case 0x5a: as cmd_ghost_unlink, plus
// the attachment slot.
void ghost_dismount(Character *c)
{
    Character *mount = c->get_mount();

    if (mount) {
        mount->mount_flags = 0;
        mount->mount_link = -1;
        mount->ai.target = -1;
        mount->flags &= ~0x20;
        c->mount_flags = 0;
        c->mount_link = -1;
        c->flags |= CF_COMBAT_READY;
        c->attachments[0].slot = 0;
    }
}

// 0x4bb870  Confidence: exact. boss_set_ride_anims case 0x5a: mounted again, stage 1 forced.
void ghost_set_ride_anims(Character *c)
{
    int anim;

    ghost_saved_trip_anim = c->anim_tripped_liedown;
    c->anim_tripped_liedown = -1;
    anim = charinfo_find_anim(c->info, "stage1_stand");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "stage1_walk");
    if (anim != -1) {
        c->anim_norm_walk = anim;
        c->anim_walk = anim;
        c->anim_norm_walk_hurt = anim;
        c->anim_walk_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "stage1_run");
    if (anim != -1) {
        c->anim_norm_run = anim;
        c->anim_run = anim;
        c->anim_norm_run_hurt = anim;
        c->anim_run_hurt = anim;
    }
    boss_ensure_mount(c);
    ghost_target_stage = 1;
    ghost_stage = 1;
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
    c->attachments[0].slot = 0x11;
    ghost_mounted = true;
}

// 0x4bb970  Confidence: exact. Spells 0x50-0x53 (fear and the three summons) are cast as moves.
bool ghost_cast_special(Character *c, int anim)
{
    if (ghost_cur_move && ghost_cur_move->spell >= SPELL_GHOST_PARALYZING_FEAR
        && ghost_cur_move->spell <= SPELL_GHOST_SUMMON_UNDEAD) {
        c->perform_spell_cast(ghost_cur_move->spell, false);
        return true;
    }
    return false;
}

// 0x4bb9a0  Confidence: exact (VC6 byte match) (a switch: the compare tree tests 0x50 first, the 0x51-0x53 body comes
// first). The summons count up ghost_minions_raised and cast at the chosen minion and point; fear goes
// at the target.
void ghost_fire_attack(Character *boss, Character *target)
{
    if (ghost_cur_move == NULL)
        return;
    switch (ghost_cur_move->spell) {
    case SPELL_GHOST_SUMMON_BONE_SERVANT:
    case SPELL_GHOST_SUMMON_BONE_MAGE:
    case SPELL_GHOST_SUMMON_UNDEAD:
        ghost_minions_raised++;
        spell_cast_execute(boss, ghost_cur_move->spell, ghost_summon_minion, &ghost_summon_pos, boss->layer, 0, 0, 0);
        break;
    case SPELL_GHOST_PARALYZING_FEAR:
        spell_cast_execute(boss, SPELL_GHOST_PARALYZING_FEAR, ghost_target, NULL, 0, 0, 0, 0);
        break;
    }
}

// 0x4bba20  Confidence: exact. Flag 1: not paralysed (fear target), 2: not silenced.
void ghost_target_flags(Character *boss, boss_target *t)
{
    if (!t->character->has_status(STATUS_PARALYZE))
        t->flags |= 1;
    if (!t->character->has_status(STATUS_SILENCE))
        t->flags |= 2;
}

// 0x4bba60  Confidence: exact. 0 / 3 / 5 the summons of stages 1 / 2 / 3, 1 stage 1 fear, 2 the third
// move of each stage, 4 the second moves of stages 2 and 3 (the class the hit dispatchers test), -1 none.
int ghost_is_own_move(Character *c)
{
    if (ghost_cur_move == NULL)
        return -1;
    if (ghost_cur_move == ghost_move_1_0)
        return 0;
    if (ghost_cur_move == ghost_move_2_0)
        return 3;
    if (ghost_cur_move == ghost_move_3_0)
        return 5;
    if (ghost_cur_move == ghost_move_1_1)
        return 1;
    if (ghost_cur_move == ghost_move_1_2 || ghost_cur_move == ghost_move_2_2 || ghost_cur_move == ghost_move_3_2)
        return 2;
    if (ghost_cur_move == ghost_move_2_1 || ghost_cur_move == ghost_move_3_1)
        return 4;
    return -1;
}

// 0x4bbae0  Confidence: exact. cur_character's AI group only (no multiplayer branch, unlike Pyrul's).
bool ghost_attack_test(Character *c)
{
    ai_group *group;
    Character *t;
    int i;

    ghost_num_hits = 0;
    group = cur_character->ai.group;
    if (group) {
        for (i = 0; i < group->count; i++) {
            t = living_from_handle(group->members[i]);
            if (t && ghost_point_in_attack_cone(t, ghost_cur_move)
                && living_combat_test_defense(&c->combat, &t->combat))
                ghost_hits[ghost_num_hits++] = group->members[i];
        }
    }
    return ghost_num_hits > 0;
}

// 0x4bbb80  Confidence: exact (VC6 byte match). Same body as pyrul_point_in_attack_cone on the ghost's points.
bool ghost_point_in_attack_cone(Character *c, boss_attack *move)
{
    boss_offense_point *pt = &ghost_offense_points[move->offense_point];
    vector3 d(c->pos.x - pt->pos.x, 0.0f, c->pos.z - pt->pos.z);

    if (d.normalize_safe_length() > move->range_max)
        return false;
    if (d.x * pt->dir.x + d.y * pt->dir.y + d.z * pt->dir.z < move->cos_max_angle)
        return false;
    return true;
}

// 0x4bbc20  Confidence: exact.
void ghost_resolve_hit(Character *c)
{
    int i;
    Character *t;

    for (i = 0; i < ghost_num_hits; i++) {
        t = living_from_handle(ghost_hits[i]);
        if (t)
            living_combat_resolve_hit_vs(&c->combat, &t->combat);
    }
}

// 0x4bbc70  Confidence: exact (VC6 byte match) (three loops, one per minion array; obj_from_handle's object read as a
// Character). Every dead, inactive minion is reset (boss_resurrect_full) and counts one summon back;
// when the count drops to 0 the wander and summon timers fire at once. Nothing checks that the count is
// positive (docs/sum-exe/bosses.md says "while it is positive"; NOTES.md).
void ghost_revive_minions()
{
    int before = ghost_minions_raised;
    Character *c;
    int i;

    for (i = 0; i < 2; i++) {
        c = (Character *)obj_from_handle(ghost_bone_servants[i]);
        if (c && (c->flags & CF_DEAD) && !(c->obj_flags & 1)) {
            ghost_minions_raised--;
            boss_resurrect_full(c);
        }
    }
    for (i = 0; i < 2; i++) {
        c = (Character *)obj_from_handle(ghost_bone_mages[i]);
        if (c && (c->flags & CF_DEAD) && !(c->obj_flags & 1)) {
            ghost_minions_raised--;
            boss_resurrect_full(c);
        }
    }
    for (i = 0; i < 2; i++) {
        c = (Character *)obj_from_handle(ghost_undead[i]);
        if (c && (c->flags & CF_DEAD) && !(c->obj_flags & 1)) {
            ghost_minions_raised--;
            boss_resurrect_full(c);
        }
    }
    if (before != 0 && ghost_minions_raised == 0) {
        ghost_think_timer.SetTimeout(0);
        ghost_summon_timer.SetTimeout(0);
    }
}

// 0x4bbd80  Confidence: exact. The level scripts' and cmd_ghost_solo's dismount: stage 3, combat on.
void ghost_set_solo(Character *rider)
{
    rider_unlink_mount(rider);
    rider->flags |= CF_COMBAT_READY;
    ghost_setup_anims_unmounted(rider);
    rider->process_anim(0.0f);
    rider->find_ground();
    ghost_combat_enabled = true;
    ghost_stage = 3;
    ghost_target_stage = 3;
}

// 0x4bbdd0  Confidence: exact (VC6 byte match) (one loop over the pair index, undead, mages, servants in that order).
// Hides every minion and takes it off the active object list (obj flag 1).
void ghost_minions_deactivate()
{
    int i;
    Character *c;

    for (i = 0; i < 2; i++) {
        c = living_from_handle(ghost_undead[i]);
        if (c) {
            c->flags |= CF_HIDDEN;
            c->obj_flags &= ~1;
        }
        c = living_from_handle(ghost_bone_mages[i]);
        if (c) {
            c->flags |= CF_HIDDEN;
            c->obj_flags &= ~1;
        }
        c = living_from_handle(ghost_bone_servants[i]);
        if (c) {
            c->flags |= CF_HIDDEN;
            c->obj_flags &= ~1;
        }
    }
}
