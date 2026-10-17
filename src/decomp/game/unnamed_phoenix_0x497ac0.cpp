// unnamed_phoenix_0x497ac0.cpp - Sum.exe v1.40, code 0x497ac0-0x499e41: the Phoenix Rider boss (Liangshan,
// Jadetemple and the random hills level): stage changes and the rebirth, the mount, the attack choice
// per stage, the reposition and dive, the special strikes, the console commands phoenix_stage /
// phoenix_unlink / phoenix_solo / phoenix_combat.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: the console initialisers (entries 278-281) and init_phoenix_timers (entry 282, 0x497cc0)
// after the serpent unit's serpent_set_solo, up to phoenix_evaluate_target (0x499e20-0x499e41); the
// tiger unit's init_tiger_timers (entry 283) follows at 0x499e50.
// Data: .bss 0x2452418-0x2452a6b (two attack pointers, the fight state object 0x2452420-0x24529f7
// (D3403), the console objects with the other attack pointers between them, phoenix_mount,
// phoenix_loaded), .data 0x59c80c-0x59c97f (console and animation strings, "phoenix_boss.tbl",
// "Phoenix Bow"). 4.5 (0x581b20) and 0.66 (0x581b24) are this unit's .rdata; the rest is pooled.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/ai/unnamed_phoenix_0x497ac0.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/effects/fx.h"
#include "engine/spells/spells.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"

#define SPELL_PHOENIX_MISSILE 0x1a    // spells.tbl indices of the three special strikes (names ours)
#define SPELL_PHOENIX_GROUND  0x1d
#define SPELL_PHOENIX_BLAST   0x1e

// ---- .bss / .data ----------------------------------------------------------------------------------
boss_attack *phoenix_move_s3_5;                               // 0x2452418
boss_attack *phoenix_move_s1_1;                               // 0x245241c
// 0x497cc0 init_phoenix_timers_thunk (jmp, static initialiser entry 282). Confidence: exact.
// 0x497cd0 init_phoenix_timers: the compiler-generated constructor of phoenix_boss (the five threat
//          timers, then the six others). Confidence: exact.
phoenix_boss phoenix;                                         // 0x2452420
boss_attack *phoenix_move_s3_4;                               // 0x2452a04
boss_attack *phoenix_move_s2_3;                               // 0x2452a08
boss_attack *phoenix_move_s3_3;                               // 0x2452a0c
boss_attack *phoenix_move_s1_0;                               // 0x2452a10
boss_attack *phoenix_move_s2_0;                               // 0x2452a14
boss_attack *phoenix_move_s3_0;                               // 0x2452a18
boss_attack *phoenix_move_s1_2;                               // 0x2452a1c
boss_attack *phoenix_move_s3_6;                               // 0x2452a20
boss_attack *phoenix_move_s2_1;                               // 0x2452a24
boss_attack *phoenix_move_s3_1;                               // 0x2452a28
boss_attack *phoenix_move_s3_2;                               // 0x2452a2c
boss_attack *phoenix_move_s2_2;                               // 0x2452a30
Character *phoenix_mount;                                     // 0x2452a54
boss_attack *phoenix_move_s2_4;                               // 0x2452a64
bool phoenix_loaded;                                          // 0x2452a68

// ---- console commands -------------------------------------------------------------------------------
// 0x497ac0 init_ccmd_phoenix_stage_thunk (jmp, static initialiser entry 278). Confidence: exact.
// 0x497ad0 init_ccmd_phoenix_stage (console_command constructor). Confidence: exact.
console_command ccmd_phoenix_stage("phoenix_stage", "Set phoenix rider's current combat stage",
                                   cmd_phoenix_stage);       // 0x24529f8

// 0x497af0  Confidence: exact.
void cmd_phoenix_stage()
{
    process_command(ARG_INT);
    if ((arg_type & ARG_INT) && command_argument_int >= 0 && command_argument_int <= 3)
        phoenix_target_stage = command_argument_int;
}

// 0x497b20 init_ccmd_phoenix_unlink_thunk (jmp, static initialiser entry 279). Confidence: exact.
// 0x497b30 init_ccmd_phoenix_unlink (console_command constructor). Confidence: exact.
console_command ccmd_phoenix_unlink("phoenix_unlink", "Unlinks phoenix to mount", cmd_phoenix_unlink);   // 0x2452a58

// 0x497b50  Confidence: exact (VC6 byte match) (the store order). The first phoenix rider in the living list.
void cmd_phoenix_unlink()
{
    Character *c;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->info->species == SPECIES_PHOENIX_RIDER) {
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
            return;
        }
    }
}

// 0x497be0  Confidence: exact. The last match (cmd_phoenix_solo, ls_rand_hills01_frame).
Character *phoenix_find_rider()
{
    Character *found = NULL;
    for (Character *c = living_list.live_next; c != &living_list; c = c->live_next)
        if (c->info->species == SPECIES_PHOENIX_RIDER)
            found = c;
    return found;
}

// 0x497c20 init_ccmd_phoenix_solo_thunk (jmp, static initialiser entry 280). Confidence: exact.
// 0x497c30 init_ccmd_phoenix_solo (console_command constructor). Confidence: exact.
console_command ccmd_phoenix_solo("phoenix_solo", "Set phoenix without horse\n", cmd_phoenix_solo);   // 0x2452a38

// 0x497c50  Confidence: exact.
void cmd_phoenix_solo()
{
    Character *c = phoenix_find_rider();
    if (c)
        phoenix_set_solo(c);
}

// 0x497c70 init_ccmd_phoenix_combat_thunk (jmp, static initialiser entry 281). Confidence: exact.
// 0x497c80 init_ccmd_phoenix_combat (console_command constructor). Confidence: exact.
console_command ccmd_phoenix_combat("phoenix_combat", "Sets phoenix rider in combat state",
                                    cmd_phoenix_combat);     // 0x2452a48

// 0x497ca0  Confidence: exact.
void cmd_phoenix_combat()
{
    phoenix_combat = true;
    print_to_console("Setting phoenix's ai state to combat\n", 0);
}

// ---- movement ---------------------------------------------------------------------------------------
// 0x497d30  Confidence: exact. Half way between the phoenix and the party leader.
void ai_state::phoenix_set_dive_point()
{
    phoenix_dive_point = (cur_character->pos + owner->pos) * 0.5f;
}

// 0x497da0  Confidence: likely (the local copies of the first disabled member). Sub-mode 0x20 setup. In
// stage 3 before a dive: a point beside two party members that are down (AI mode (8, 0x2d)), else the
// one that is down or the phoenix itself is the dive point; otherwise a random navigable point 15-20 m
// (stage 1) or 5-8 m away. The mount travels there when there is one.
// When the side point has no path the second try paths to the negated point (-dest), not to the
// mirrored side point (NOTES.md).
bool ai_state::phoenix_reposition_update()
{
    vector3 centre;
    float lo, hi;

    if ((phoenix_flags & PHOENIX_F_DIVE) && phoenix_stage == 3) {
        Character *down[4];
        int n = 0;
        lo = 3.0f;
        hi = 6.0f;
        for (Player *p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (c && c->ai.is_mode(AI_STATE_DISABLED, 0x2d))
                down[n++] = c;
        }
        if (n > 1) {
            vector3 a = down[0]->pos;
            vector3 b = down[1]->pos;
            vector3 mid;
            mid = (a + b) * 0.5f;
            vector3 dir(-(a.z - mid.z), 0, a.x - mid.x);
            dir.normalize();
            vector3 dest = mid + dir * 4.5f;
            if (path_find_new(&owner->pos, owner->layer, &dest, owner->layer, owner->info->size, 0.0f)) {
                set_mode(AI_STATE_TRAVELING, -1);
            } else {
                dest = -dest;
                if (path_find_new(&owner->pos, owner->layer, &dest, owner->layer, owner->info->size, 0.0f))
                    set_mode(AI_STATE_TRAVELING, -1);
            }
            centre = mid;
        } else if (n == 1) {
            centre = down[0]->pos;
        } else {
            centre = owner->pos;
        }
        phoenix_dive_point = centre;
        if (is_mode(AI_STATE_TRAVELING, -1)) {
            phoenix_flags |= PHOENIX_F_REPOS_PATH;
            phoenix_wander_timer.SetTimeout(5000);
            return true;
        }
    } else {
        if (phoenix_stage == 1) {
            hi = 20.0f;
            lo = 15.0f;
        } else {
            hi = 8.0f;
            lo = 5.0f;
        }
        centre = owner->pos;
    }
    float range = hi - lo;
    int tries = 7;
    do {
        vector3 p;
        p.random_direction();
        p *= frand() * range + lo;
        p = centre + p;
        if (pf_point_is_navigable(&p, owner->info->size, owner->layer)
            && path_find_new(&owner->pos, owner->layer, &p, owner->layer, owner->info->size, 0.0f)) {
            Character *m = owner->get_mount();
            if (m)
                m->ai.set_mode(AI_STATE_MOUNT_TRAVEL, -1);
            else
                set_mode(AI_STATE_TRAVELING, -1);
            phoenix_wander_timer.SetTimeout(5000);
            return true;
        }
    } while (tries-- != 0);
    phoenix_timer_c4.SetTimeout(500);
    return false;
}

// 0x4981a0  Confidence: exact.
void ai_state::phoenix_wander_update()
{
    if (!follow_path(0) || phoenix_wander_timer.elapsed()) {
        phoenix_timer_c4_set_random(5000, 10000);
        set_mode(AI_STATE_COMBAT, 0x16);
    }
}

// 0x4981e0  Confidence: exact.
void phoenix_timer_c4_set_random(int lo, int hi)
{
    phoenix_timer_c4.SetTimeout((int)(frand() * (hi - lo)) + lo);
}

// 0x498210  Confidence: exact (VC6 byte match) (statement order). Sub-mode 0x3a.
void ai_state::phoenix_end_move_3a()
{
    phoenix_cur_attack = NULL;
    phoenix_timer_cc.SetTimeout(20000);
    phoenix_timer_c0.SetTimeout(0);
    phoenix._unk598 = -1;
    set_mode(AI_STATE_COMBAT, 0xf);
}

// 0x498250  Confidence: exact (VC6 byte match). Sub-mode 0x3b.
void ai_state::phoenix_end_move_3b()
{
    phoenix_cur_attack = NULL;
    phoenix_timer_d0.SetTimeout(10000);
    phoenix_timer_c0.SetTimeout(0);
    phoenix._unk598 = -1;
    phoenix_timer_cc.invalidate();
    set_mode(AI_STATE_COMBAT, 0xf);
}

// 0x4982a0  Confidence: exact (VC6 byte match). As serpent_seat_rider, but the animation time is read from and written
// back to the mount (the rider's time is never set; NOTES.md).
bool ai_state::phoenix_seat_rider()
{
    if (phoenix_mount)
        boss_rider_random_sound(&phoenix_mount->pos);
    if (phoenix_has_rider) {
        Character *m = owner->get_mount();
        if (m != NULL) {
            m->ai.target = target;
            float t;
            m->instance->get_set_time(&t, false);
            m->instance->get_set_time(&t, true);
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

// ---- setup -----------------------------------------------------------------------------------------
// 0x498420  Confidence: exact (VC6 byte match, match31; was likely) (the has_rider default before the loaded test). Once per level (until
// phoenix_unload): the stage (3 on foot in Liangshan / Jadetemple), the timers, the table, the named
// attack records, the Phoenix Bow, the flinch moves, the seat.
void ai_state::phoenix_boss_load()
{
    phoenix_has_rider = true;
    phoenix_dismount_stage_reached = false;
    phoenix_stage_trigger = false;
    if (phoenix_loaded == true)
        return;
    phoenix_loaded = true;
    if (stricmp(level_filename, "Liangshan") == 0 || stricmp(level_filename, "Jadetemple") == 0) {
        phoenix_stage = 3;
        phoenix_target_stage = 3;
    } else {
        phoenix_stage = 1;
        phoenix_target_stage = 0;
    }
    phoenix_flags = 0;
    phoenix_timer_c0.invalidate();
    phoenix_timer_c4.invalidate();
    phoenix_timer_cc.invalidate();
    phoenix_timer_d0.SetTimeout(0);
    phoenix_timer_d4.SetTimeout(0);
    phoenix.ranged_count = 0;
    phoenix._unk598 = -1;
    phoenix_ground_effect = -1;
    owner->flags |= CF_COMBAT_READY;
    if (stricmp(level_filename, "Liangshan") == 0 || stricmp(level_filename, "Jadetemple") == 0)
        phoenix_setup_anims_wrapper(owner);
    else
        phoenix_set_stage1_anims(owner);
    for (int i = 0; i < BOSS_MAX_THREATS; i++)
        phoenix_threat_table[i].handle = -1;

    phoenix_num_offense_points = 0;
    for (int k = 0; k < 10; k++) {
        const char *name = offense_point_name(k);
        ca_character *ch = owner->instance->character;
        int tag = ch->find_mesh_tag(name);
        if (tag != -1) {
            phoenix.offense_points[phoenix_num_offense_points].tag = tag;
            phoenix_num_offense_points++;
        }
    }
    boss_ai_parse_table("phoenix_boss.tbl", PHOENIX_STAGES, phoenix.attacks, phoenix_num_offense_points);
    phoenix_move_s1_0 = &phoenix.attacks[0][0];
    phoenix_move_s1_1 = &phoenix.attacks[0][1];
    phoenix_move_s1_2 = &phoenix.attacks[0][2];
    phoenix_move_s2_0 = &phoenix.attacks[1][0];
    phoenix_move_s2_1 = &phoenix.attacks[1][1];
    phoenix_move_s2_2 = &phoenix.attacks[1][2];
    phoenix_move_s2_3 = &phoenix.attacks[1][3];
    phoenix_move_s2_4 = &phoenix.attacks[1][4];
    phoenix_move_s3_0 = &phoenix.attacks[2][0];
    phoenix_move_s3_1 = &phoenix.attacks[2][1];
    phoenix_move_s3_2 = &phoenix.attacks[2][2];
    phoenix_move_s3_3 = &phoenix.attacks[2][3];
    phoenix_move_s3_4 = &phoenix.attacks[2][4];
    phoenix_move_s3_5 = &phoenix.attacks[2][5];
    phoenix_move_s3_6 = &phoenix.attacks[2][6];

    int type = item_type_find("Phoenix Bow");
    item_info *old1, *old2;
    character_equip_item(&old1, &old2, owner, 5, item_create_weapon(type));
    if (old1)
        item_free(old1);
    if (old2)
        item_free(old2);
    owner->attachments[0].item_type = type;
    owner->attachments[0].slot = 0;
    owner->num_attachments = 1;
    phoenix_rider_set_flinch((move_info *)owner->info->moves[4]);
    if (stricmp(level_filename, "Liangshan") != 0 && stricmp(level_filename, "Jadetemple") != 0)
        phoenix_seat_rider();
    phoenix_mount = owner->get_mount();
}

// 0x498720  Confidence: exact (VC6 byte match) (the buffer sizes). As serpent_rider_set_flinch.
void phoenix_rider_set_flinch(move_info *moves)
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
            phoenix.flinch_area[stage] = m;
        else if (stricmp(p, "status") == 0)
            phoenix.flinch_status[stage] = m;
        else
            sprintf(msg, "Unable to set Phoenix Rider flinch for %s -- bad $Action\n", m->anim_file);
    }
    phoenix.flinch_area[2] = phoenix.flinch_area[1];
    phoenix.flinch_status[2] = phoenix.flinch_status[1];
}

// 0x498840  Confidence: exact. Mounted (stage 1): the mount, the stage1_* animations.
void phoenix_set_stage1_anims(Character *c)
{
    int anim;

    boss_ensure_mount(c);
    phoenix_saved_move = c->anim_tripped_liedown;
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

// 0x498900  Confidence: exact.
void phoenix_setup_anims_wrapper(Character *c)
{
    phoenix_setup_anims(c);
}

// 0x498910  Confidence: exact. On foot: ready / walk / run, off the mount.
void phoenix_setup_anims(Character *c)
{
    int anim;

    anim = charinfo_find_anim(c->info, "ready");
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
}

// 0x4989d0  Confidence: exact. ai_state_shutdown (boss_ai_unload).
void phoenix_unload()
{
    phoenix_loaded = false;
}

// ---- attack choice ---------------------------------------------------------------------------------
// 0x4989e0  Confidence: exact. Without a move the melee record goes back to state 0 first; then, in
// combat, the attack is cleared and the AI enters combat.
void ai_state::phoenix_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != 0) {
        owner->combat.state = 0;
        return;
    }
    if (phoenix_combat == true) {
        phoenix_cur_attack = NULL;
        enter_combat();
    }
}

// 0x498a20  Confidence: unsure (the shape of the target re-picks; the frand calls whose result is not
// used are kept as statements). Returns 0 out of combat or without a target, 5 while an attack plays,
// 1 with a pending sub-mode (0x20 reposition, 0x3a / 0x3b the end moves), else 3 after choosing
// phoenix_cur_attack (ai_set_mode(3, 0x10) for the ranged moves, (3, 0x16) for the others, no mode
// change for the dive and the timed moves).
int ai_state::phoenix_choose_attack()
{
    boss_target list[5];
    matrix tag_orient;
    vector3 tag_pos;
    int i, n, cur, h, p, lo, hi;
    float r;

    if (!phoenix_combat)
        return 0;
    bool busy = owner->combat.is_busy() == true;     // computed, not used
    (void)busy;
    if (phoenix_cur_attack)
        return 5;
    if (is_mode(AI_STATE_COMBAT, 0x20))
        return 5;
    if (phoenix_ground_effect != -1 && fx_is_finished(phoenix_ground_effect, NULL))
        phoenix_ground_effect = -1;
    for (i = 0; i < phoenix_num_offense_points; i++) {
        owner->instance->get_bone_world_transform(phoenix.offense_points[i].tag, &owner->orient, &owner->pos,
                                                  &tag_orient, &tag_pos);
        phoenix.offense_points[i].up = tag_orient.uvec;
        phoenix.offense_points[i].pos = tag_pos + owner->centre_offset;
    }
    int attackers = count_attackers();
    n = boss_target_list_build(owner, list, phoenix_threat_table);
    if (n == 0)
        return 0;
    cur = boss_target_list_find(list, n, target);
    if (!phoenix_timer_c0.valid()) {
        phoenix_timer_c0_set_random(20000, 30000);
        phoenix_timer_c4_set_random(5000, 10000);
    }
    bool c0_elapsed = phoenix_timer_c0.elapsed();

    if (phoenix_stage == 1) {
        h = target;
        if (phoenix_flags & PHOENIX_F_REPOSITION)
            goto reposition;
        if (phoenix_flags & PHOENIX_F_DIVE) {
            phoenix_set_dive_point();
            phoenix_cur_attack = phoenix_move_s1_2;
            goto dive;
        }
        if (c0_elapsed || cur == -1) {
            p = phoenix_pick_weighted_move(list, n, h, false, false);
            if (p != -1) {
                h = p;
                phoenix_timer_c0_set_random(20000, 30000);
            }
        }
        if (boss_target_list_find(list, n, h) == -1)
            return 0;
        phoenix_target_handle = h;
        if (phoenix_timer_d4.elapsed()) {
            r = frand();
            if (attackers >= 2 || r < 0.05f) {
                phoenix_flags |= PHOENIX_F_REPOSITION;
                phoenix_cur_attack = phoenix_move_s1_1;
                phoenix_timer_d4.SetTimeout(50000);
                return 3;
            }
        }
        phoenix_cur_attack = phoenix_move_s1_0;
        set_mode(AI_STATE_COMBAT, 0x10);
        return 3;
    }

    if (phoenix_stage == 2) {
        h = target;
        if (c0_elapsed || cur == -1) {
            r = frand();
            lo = 20000;
            hi = 30000;
            if (!phoenix_timer_cc.valid()) {
                if ((attackers == 0 || r < 0.3f) && (p = phoenix_pick_weighted_move(list, n, h, false, true)) != -1) {
                    lo = 5000;
                    hi = 10000;
                } else {
                    p = phoenix_pick_weighted_move(list, n, h, false, false);
                }
            } else {
                p = phoenix_pick_weighted_move(list, n, h, true, false);
                if (p == -1)
                    p = phoenix_pick_weighted_move(list, n, h, false, false);
                lo = 3000;
                hi = 5000;
            }
            if (p != -1) {
                h = p;
                phoenix_timer_c0_set_random(lo, hi);
            }
        }
        cur = boss_target_list_find(list, n, h);
        if (cur == -1)
            return 0;
        phoenix_target_handle = h;
        if (phoenix_timer_cc.valid()) {
            if (phoenix_timer_cc.elapsed()) {
                phoenix_cur_attack = phoenix_move_s2_3;
                phoenix.pending_submode = 0x3b;
                return 1;
            }
            phoenix_cur_attack = phoenix_move_s2_4;
            return 3;
        }
        if (list[cur].flags & PHOENIX_TF_FAR) {
            if (frand() < 0.3f / (phoenix.ranged_count * 0.25f + 1.0f)) {
                phoenix.ranged_count++;
                phoenix_cur_attack = phoenix_move_s2_2;
                return 3;
            }
            phoenix_cur_attack = phoenix_move_s2_0;
            set_mode(AI_STATE_COMBAT, 0x10);
            return 3;
        }
        phoenix.ranged_count = 0;
        if (phoenix_timer_d0.elapsed() && frand() < 0.3f) {
            phoenix_cur_attack = phoenix_move_s2_3;
            phoenix.pending_submode = 0x3a;
            return 1;
        }
        phoenix_cur_attack = phoenix_move_s2_1;
        set_mode(AI_STATE_COMBAT, 0x16);
        return 3;
    }

    if (phoenix_flags & PHOENIX_F_REPOSITION) {
reposition:
        phoenix_flags = (phoenix_flags & ~PHOENIX_F_REPOSITION) | PHOENIX_F_DIVE;
        phoenix.pending_submode = 0x20;
        return 1;
    }
    if (phoenix_flags & PHOENIX_F_DIVE) {
        phoenix_cur_attack = phoenix_move_s3_6;
dive:
        phoenix_flags &= ~PHOENIX_F_DIVE;
        phoenix_timer_c0.SetTimeout(0);
        return 3;
    }
    h = target;
    if (c0_elapsed) {
        frand();
        lo = 20000;
        hi = 30000;
        if (!phoenix_timer_cc.valid()) {
            if (attackers == 0 && (p = phoenix_pick_weighted_move(list, n, h, false, true)) != -1) {
                lo = 5000;
                hi = 10000;
            } else {
                p = phoenix_pick_weighted_move(list, n, h, false, false);
            }
        } else {
            p = phoenix_pick_weighted_move(list, n, h, true, false);
            if (p == -1)
                p = phoenix_pick_weighted_move(list, n, h, false, false);
            lo = 3000;
            hi = 5000;
        }
        if (p != -1) {
            h = p;
            phoenix_timer_c0_set_random(lo, hi);
        }
    }
    cur = boss_target_list_find(list, n, h);
    if (cur == -1)
        return 0;
    phoenix_target_handle = h;
    if (phoenix_timer_cc.valid()) {
        if (phoenix_timer_cc.elapsed()) {
            phoenix_cur_attack = phoenix_move_s3_3;
            phoenix.pending_submode = 0x3b;
            return 1;
        }
        phoenix_cur_attack = phoenix_move_s3_4;
        return 3;
    }
    if (list[cur].flags & PHOENIX_TF_FAR) {
        r = frand();
        if (phoenix_ground_effect != -1)
            r = 1.0f;
        if (r < 0.3f / (phoenix.ranged_count * 0.25f + 1.0f)) {
            phoenix.ranged_count++;
            phoenix_cur_attack = phoenix_move_s3_2;
            return 3;
        }
        phoenix_cur_attack = phoenix_move_s3_0;
        set_mode(AI_STATE_COMBAT, 0x10);
        return 3;
    }
    if (phoenix_timer_d4.elapsed()) {
        r = frand();
        if (attackers >= 2 || r < 0.05f) {
            phoenix_cur_attack = phoenix_move_s3_5;
            phoenix_dive_point = owner->pos;
            phoenix_flags |= PHOENIX_F_REPOSITION;
            phoenix_timer_d4.SetTimeout(50000);
            return 3;
        }
    }
    if (phoenix_timer_d0.elapsed()) {
        frand();
        phoenix_cur_attack = phoenix_move_s3_3;
        phoenix.pending_submode = 0x3a;
        return 1;
    }
    phoenix_cur_attack = phoenix_move_s3_1;
    set_mode(AI_STATE_COMBAT, 0x16);
    return 3;
}

// 0x499220  Confidence: exact.
void phoenix_timer_c0_set_random(int lo, int hi)
{
    phoenix_timer_c0.SetTimeout((int)(frand() * (hi - lo)) + lo);
}

// 0x499250  Confidence: exact (VC6 byte match) (the skip test keeps the binary's three-way OR; the list entries always
// have a character). Cumulative weights 2 + 0.1 * (minus the summed threat of the list) minus the
// candidate's own threat (3 * [0] + [1]); the current target x1.2, or 0 when excluded; a far-only pick
// skips the near ones.
int phoenix_pick_weighted_move(boss_target *list, int count, int cur_target, bool exclude_cur, bool far_only)
{
    float total = 0;
    float threat_sum = 0;
    int i, k;

    for (i = 0; i < count; i++) {
        if (list[i].character) {
            k = phoenix_threat_find(list[i].character->handle);
            if (k != -1)
                threat_sum -= phoenix_threat_table[k].threat[0] + phoenix_threat_table[k].threat[1];
        }
    }
    for (i = 0; i < count; i++) {
        boss_target *t = &list[i];
        bool ok = true;
        bool is_cur = t->character && t->character->handle == cur_target;
        if (far_only == true && !(t->flags & PHOENIX_TF_FAR))
            ok = false;
        Character *c = t->character;
        if (c == NULL && !ok && !is_cur) {
            t->score = total;
            continue;
        }
        float base = threat_sum * 0.1f + 2.0f;
        float w;
        k = phoenix_threat_find(c->handle);
        if (k != -1)
            w = base - (phoenix_threat_table[k].threat[0] * 3.0f + phoenix_threat_table[k].threat[1]);
        else
            w = base;
        if (is_cur) {
            w *= 1.2f;
            if (exclude_cur)
                w = 0.0f;
        }
        total += w;
        t->score = total;
    }
    for (i = 0; i < count; i++)
        list[i].score /= total;
    float r = frand();
    int pick = -1;
    for (i = 0; i < count; i++) {
        if (r <= list[i].score) {
            pick = i;
            break;
        }
    }
    if (pick != -1)
        return list[pick].character->handle;
    return -1;
}

// 0x4993f0  Confidence: exact.
int phoenix_threat_find(int handle)
{
    for (int i = 0; i < BOSS_MAX_THREATS; i++)
        if (phoenix_threat_table[i].handle == handle)
            return i;
    return -1;
}

// 0x499410  Confidence: exact (the choose result is a switch; the new handle is a local, D7700; case order
// from the bodies: 15, 22/23, 0x3a, 0x3b, 0x20, 16; the table starts at -1). Nothing while the rebirth
// move plays.
void ai_state::phoenix_ai_frame()
{
    if (phoenix_flags & PHOENIX_F_REBIRTH)
        return;
    Character *t = living_from_handle(target);
    if (t && (t->flags & CF_HIDDEN))
        t = NULL;
    int r = phoenix_choose_attack();
    switch (r) {
    case 0:
        go_idle();
        return;
    case 1: {
        int pending = phoenix.pending_submode;
        if (pending == 0x20) {
            if (!phoenix_reposition_update())
                return;
            phoenix_timer_c4_set_random(15000, 25000);
            set_mode(AI_STATE_COMBAT, 0x20);
        } else if (pending == 0x3a || pending == 0x3b) {
            set_mode(AI_STATE_COMBAT, pending);
        }
        break;
    }
    }
    int h = phoenix_target_handle;
    if (h != target) {
        target = h;
        owner->combat.target = h;
        flags |= 0x10;
        t = living_from_handle(target);
    }
    if (t == NULL || (t->flags & CF_HIDDEN)) {
        go_idle();
        return;
    }
    if (r == 3)
        combat_approach(t);
    switch (submode) {
    case 15:
        combat_melee_attack(t);
        return;
    case 22:
    case 23:
        combat_close_in(t);
        return;
    case 0x3a:
        phoenix_end_move_3a();
        return;
    case 0x3b:
        phoenix_end_move_3b();
        return;
    case 0x20:
        phoenix_wander_update();
        return;
    case 16:
        combat_ranged_attack(t);
        return;
    case -1:
        return;
    }
}

// ---- stages -----------------------------------------------------------------------------------------
// 0x4995b0  Confidence: exact. The rebirth: the move (type 9, id 1) once, hidden; when it ends, half the
// maximum HP and stage 3.
void ai_state::phoenix_stage_transition_move()
{
    if (emerge_move == -1) {
        move_info *m = owner->find_move(9, 1);
        if (m == NULL)
            return;
        emerge_move = owner->play_move((int)m, 1.0f, true, true, false);
        disable_timer.SetTimeout(0);
        go_idle();
        phoenix_flags |= PHOENIX_F_REBIRTH;
        owner->flags |= CF_HIDDEN;
        return;
    }
    if (emerge_move >= 0 && owner->move_finished(emerge_move)) {
        owner->hp = (float)owner->get_max_stat(0) * 0.5f;
        phoenix_stage = 3;
        phoenix_flags = (phoenix_flags & ~PHOENIX_F_REBIRTH) | PHOENIX_F_REBORN;
        owner->flags &= ~CF_HIDDEN;
    }
}

// 0x499690  Confidence: exact (VC6 byte match) (the case order of the forced stage). Every frame: the rebirth at 0 HP
// (once), a forced stage, else by HP (below 30 % or reborn stage 3, below 66 % stage 2 once the level
// script answered the dismount request).
void ai_state::phoenix_check_stage()
{
    Character *c = owner;
    float frac = c->hp / (float)c->get_max_stat(0);
    unsigned char f = (unsigned char)phoenix_flags;
    if (owner->hp <= 0.0f && !(f & PHOENIX_F_REBORN)) {
        if (!(f & PHOENIX_F_REBIRTH))
            emerge_move = -1;
        phoenix_stage_transition_move();
        return;
    }
    if (phoenix_target_stage != 0) {
        if (phoenix_stage == phoenix_target_stage)
            return;
        switch (phoenix_target_stage) {
        default:
            phoenix_target_stage = 0;
            return;
        case 3:
            phoenix_setup_anims_wrapper(owner);
            phoenix_stage = 3;
            return;
        case 2:
            phoenix_setup_anims(owner);
            phoenix_stage = 2;
            return;
        case 1:
            phoenix_set_stage1_anims(owner);
            phoenix_stage = 1;
            return;
        }
    }
    if (frac < 0.3f || (f & PHOENIX_F_REBORN)) {
        if (phoenix_stage != 3) {
            phoenix_setup_anims_wrapper(owner);
            phoenix_stage = 3;
        }
        return;
    }
    if (frac < 0.66f) {
        if (phoenix_stage == 2)
            return;
        if (!phoenix_dismount_stage_reached) {
            phoenix_dismount_stage_reached = true;
            phoenix_stage_trigger = false;
            return;
        }
        if (phoenix_stage_trigger) {
            phoenix_setup_anims(owner);
            phoenix_stage = 2;
        }
        return;
    }
    if (phoenix_stage != 1) {
        phoenix_set_stage1_anims(owner);
        phoenix_stage = 1;
    }
}

// ---- the boss_ai.cpp hooks --------------------------------------------------------------------------
// 0x499810  Confidence: exact. The three ranged moves report their range (boss_get_attack_range).
bool phoenix_get_attack_range(Character *c, float *out)
{
    boss_attack *a = phoenix_cur_attack;
    if (a == phoenix_move_s1_0 || a == phoenix_move_s2_0 || a == phoenix_move_s3_0) {
        *out = a->range_max;
        return true;
    }
    return false;
}

// 0x499840  Confidence: exact. boss_cast_special_dispatch: the attack's spell (the ground strike at
// the dive point).
bool phoenix_cast_special(Character *c, int anim)
{
    if (phoenix_cur_attack->spell == -1)
        return false;
    if (phoenix_cur_attack->spell == SPELL_PHOENIX_GROUND)
        c->ai.cast_point = phoenix_dive_point;
    c->perform_spell_cast(phoenix_cur_attack->spell, false);
    return true;
}

// 0x499890  Confidence: exact (VC6 byte match) (the effect search as a for loop with a break). boss_attack_trigger_dispatch:
// 0x1a fires the missile of the effect the move started, 0x1d places the ground effect facing the
// cast point, 0x1e knocks the party down.
// GAME BUG (NOTES.md): the 0x1e blast tests the distance between the target and the phoenix (not each
// party member) against the spell's distance, and hits the target once per party member; every party
// member is knocked down when the target is in range.
void phoenix_special_strike(Character *boss, Character *target)
{
    if (phoenix_cur_attack == NULL)
        return;
    switch (phoenix_cur_attack->spell) {
    case SPELL_PHOENIX_BLAST: {
        float r2 = spells[SPELL_PHOENIX_BLAST].distance * spells[SPELL_PHOENIX_BLAST].distance;
        for (Player *p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (c == NULL)
                continue;
            vector3 d = target->pos - boss->pos;
            if (d.y * d.y + d.x * d.x + d.z * d.z < r2) {
                fx_create_spell_effect_facing(SPELL_PHOENIX_BLAST, boss->handle, target->handle);
                target->get_hit_by_spell(SPELL_PHOENIX_BLAST, boss->handle, NULL, NULL, 0, NULL, 0.0f, false);
                c->ai.knock_down((int)(frand_range(5.0f, 10.0f) * 1000.0f));
            }
        }
        break;
    }
    case SPELL_PHOENIX_GROUND: {
        vector3 dir;
        dir = vector3(boss->ai.cast_point.x - boss->pos.x, 0, boss->ai.cast_point.z - boss->pos.z);
        dir.normalize();
        matrix m;
        m.set_from_forward_up(dir, boss->orient.uvec);
        phoenix_ground_effect = spell_place_ground_effect(phoenix_cur_attack->spell, boss->handle,
                                                          &boss->ai.cast_point, boss->ai.cast_arg, &m);
        break;
    }
    case SPELL_PHOENIX_MISSILE: {
        int fx = -1;
        for (effect_object *e = fx_active_list.fx_next; e != &fx_active_list; e = e->fx_next) {
            if (e->owner == boss->handle && e->source == boss->cur_anim) {
                fx = e->handle;
                break;
            }
        }
        spell_fire_missile(SPELL_PHOENIX_MISSILE, fx, boss, target, NULL, NULL);
        break;
    }
    }
}

// 0x499a90  Confidence: exact. The ranged move (type 0) of the stage: id 4 mounted, else 2.
move_info *phoenix_select_ranged_move(Character *c)
{
    if (phoenix_stage == 1)
        return charinfo_find_move(c->info, 0, 4, true);
    return charinfo_find_move(c->info, 0, 2, true);
}

// 0x499ad0  Confidence: likely. As serpent_anim_remap_in.
void phoenix_anim_remap_in(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    Character *mount = c->get_mount();
    if (mount == NULL)
        return;
    int ready = 0;
    Character *r = mount->get_rider();
    if (r)
        ready = r->flags & CF_COMBAT_READY;
    phoenix_anim_remap_count = set->count;
    for (int i = 0; i < set->count; i++) {
        int a = set->anim[i];
        if (a == mount->anim_stand) {
            if (ready)
                phoenix.remap_slot[i] = 3;
            else
                phoenix.remap_slot[i] = 0;
        } else if (a == mount->anim_walk) {
            phoenix.remap_slot[i] = 4;
        } else if (a == mount->anim_run) {
            phoenix.remap_slot[i] = 5;
        } else {
            continue;
        }
        phoenix.remap_weight[i] = set->weight[i];
    }
}

// 0x499b80  Confidence: exact. LOGIC FIX (D7701, as D6400): the slots get the character's own loco anims (EBX,
// the argument, at 0x499bb9); the mount is only tested for NULL. The source copied mount->loco_anims.
void phoenix_anim_remap_out(Character *c, int *anims)
{
    loco_blend_set *set = (loco_blend_set *)anims;
    Character *mount = c->get_mount();
    if (mount == NULL)
        return;
    for (int i = 0; i < phoenix_anim_remap_count; i++) {
        set->anim[i] = c->loco_anims[phoenix.remap_slot[i]];
        set->weight[i] = phoenix.remap_weight[i];
    }
}

// 0x499be0  Confidence: exact (VC6 byte match) (the ready_hurt lookup's result is overwritten before use, as in the
// binary: its if block is missing). boss_set_foot_anims: stage 3.
void phoenix_set_foot_anims(Character *c)
{
    int anim;

    phoenix_has_rider = false;
    anim = charinfo_find_anim(c->info, "ready");
    if (anim != -1) {
        c->anim_stand = anim;
        c->anim_ready = anim;
        c->anim_stand_hurt = anim;
        c->anim_ready_hurt = anim;
    }
    anim = charinfo_find_anim(c->info, "ready_hurt");
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
    phoenix_stage = 3;
    phoenix_target_stage = 3;
}

// 0x499ce0  Confidence: exact. boss_set_ride_anims: stage 1 on the mount.
void phoenix_set_ride_anims(Character *c)
{
    int anim;

    phoenix_saved_move = c->anim_tripped_liedown;
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
    phoenix_target_stage = 1;
    phoenix_stage = 1;
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
    phoenix_has_rider = true;
}

// 0x499dd0  Confidence: exact. cmd_phoenix_solo and the level scripts.
void phoenix_set_solo(Character *rider)
{
    rider_unlink_mount(rider);
    rider->flags |= CF_COMBAT_READY;
    phoenix_setup_anims_wrapper(rider);
    rider->process_anim(0.0f);
    rider->find_ground();
    phoenix_combat = true;
    phoenix_stage = 3;
    phoenix_target_stage = 3;
}

// 0x499e20  Confidence: exact. boss_target_filter_dispatch.
void phoenix_evaluate_target(Character *boss, boss_target *t)
{
    if (t->character && t->dist > 2.5f)
        t->flags |= PHOENIX_TF_FAR;
}
