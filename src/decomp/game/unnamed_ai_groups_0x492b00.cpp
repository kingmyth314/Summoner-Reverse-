// unnamed_ai_groups_0x492b00.cpp - Sum.exe v1.40, code 0x492b00-0x493e41: the AI groups (ten groups of up
// to six members that share an enemy list and think together every 1-2.5 s: who attacks whom, the
// casters' support spells) and the party_controller_check console toggle.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs fn). Target: VC++ 6.0 (cl 12.00.8966), /O2 like the
// other optimised units (no frame pointer, 0x90 padding between functions).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: the static initialiser of the group timers (0x492b00, entry 263) follows levelrender.cpp's
// last function (atexit_nop_492af0); the unit ends with cmd_party_controller_check (0x493d90-0x493e41);
// the cutscene camera unit's first initialiser (entry 265) follows at 0x493e50.
// Data: .bss 0x243e040-0x243e3d7 (ai_groups, the console object 0x243e388, ai_think_enemies,
// ai_think_num_members, ai_think_members), .data 0x59c594-0x59c5ef (Party_controller_check and the
// console strings). The float literals 0.4 (0x581ad8), 30.0 (0x581adc) and 0.71 (0x581ae0) are in
// .rdata just before the camera sequence vtable (0x581ae4); 0.25, 3.0, 10.0, 0.0 are pooled.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define CHAR_TYPE_PARTY 7             // obj +0xc of a party member
#define CHAR_TYPE_4     4             // skipped by the per-style passes of ai_group_combat_think

// Spells of the caster pass (spells.tbl indices; names not checked).
#define SPELL_GROUP_SUPPORT  0x15     // cast without a target when more than two members qualify
#define SPELL_MEMBER_SUPPORT 0x13     // cast at one member that is not blessed (status 4)

// ---- .bss / .data --------------------------------------------------------------------------------
// 0x492b00 init_gametimer_array_243e090_thunk (jmp, static initialiser entry 263). Confidence: exact.
// 0x492b10 init_gametimer_array_243e090: the compiler-generated array constructor of ai_groups (the
//          GameTimer constructor of each +0x50 inlined as a call loop, stride 0x54). Confidence: exact.
ai_group ai_groups[10];                                       // 0x243e040
Character *ai_think_enemies[10];                              // 0x243e394
int ai_think_num_members;                                     // 0x243e3bc
Character *ai_think_members[6];                               // 0x243e3c0
bool Party_controller_check = true;                           // 0x59c594

// 0x492b30  Confidence: exact.
void ai_groups_reset()
{
    for (int i = 0; i < 10; i++) {
        ai_groups[i].count = 0;
        ai_groups[i].flags = 0;
        ai_groups[i].timer.invalidate();
    }
}

// 0x492b60  Confidence: exact. state_game_enter: the party groups think at once, the others are emptied.
void ai_groups_restart_timers()
{
    for (int i = 0; i < 10; i++) {
        if (ai_groups[i].count > 0 && (ai_groups[i].flags & AI_GROUP_PARTY)) {
            ai_groups[i].timer.SetTimeout(0);
        } else {
            ai_groups[i].timer.invalidate();
            ai_groups[i].count = 0;
        }
    }
}

// 0x492ba0  Confidence: exact. The first free group, with the character as leader and first member.
ai_group *ai_group_create(Character *c)
{
    int i;
    for (i = 0; i < 10; i++)
        if (ai_groups[i].count == 0)
            break;
    if (i == 10)
        return NULL;

    ai_group *g = &ai_groups[i];
    g->count++;
    g->members[0] = c->handle;
    g->leader = c->handle;
    g->timer.SetTimeout(0);
    c->ai.group = g;
    if (c->type == CHAR_TYPE_PARTY)
        g->flags |= AI_GROUP_PARTY;
    if (game_flags & GF_MULTIPLAYER) {
        if (((g->flags & AI_GROUP_PARTY) && c == cur_character)
            || (!(g->flags & AI_GROUP_PARTY) && mp_local_player && (mp_local_player->flags & 1)))
            g->flags |= AI_GROUP_LOCAL;
    }
    return g;
}

// 0x492c50  Confidence: exact.
void ai_group_add_member(ai_group *g, Character *c)
{
    if (g->count == 6)
        return;
    for (int i = 0; i < g->count; i++)
        if (g->members[i] == c->handle)
            return;
    g->members[g->count] = c->handle;
    g->count++;
    c->ai.group = g;
}

// 0x492ca0  Confidence: exact. The -1 after the shift goes to members[j] with j == 6 (+0x1c, the enemy
// count) whenever the member was not the last slot: the source indexes with the loop variable after
// the loop (NOTES.md, bug list).
void ai_group_remove_member(Character *c)
{
    ai_group *g = c->ai.group;
    int i, j;

    if (g == NULL)
        return;
    c->ai.group = NULL;
    if (g->count == 1) {
        g->flags = 0;
        g->count = 0;
        g->leader = -1;
        return;
    }
    for (i = 0; i < g->count; i++)
        if (g->members[i] == c->handle)
            break;
    for (j = i + 1; j < 6; j++)
        g->members[j - 1] = g->members[j];
    g->members[j] = -1;
    g->count--;
    if (g->leader == c->handle)
        g->leader = g->members[0];
}

// 0x492d30  Confidence: exact. ai_do_frame (skipped while a cutscene script runs).
void ai_groups_frame()
{
    for (ai_group *g = ai_groups; g < ai_groups + 10; g++) {
        if (!g->timer.elapsed())
            continue;
        if ((game_flags & GF_MULTIPLAYER) && !(g->flags & AI_GROUP_LOCAL))
            continue;
        g->flags &= ~AI_GROUP_COMBAT_SET;
        ai_group_collect_enemies(g);
        ai_group_combat_think(g);
        if (g->flags & AI_GROUP_FIGHTING)
            g->timer.SetTimeout(1000);
        else
            g->timer.SetRandomTimeout(1500, 2500);
    }
}

// 0x492db0  Confidence: likely (the AP fraction: the max-AP call before the AP load). First the
// per-member upkeep, then one pass per combat style: 0 melee, 1 support, 2 range / 4 caster, 3 healer
// / 5 healer-caster (ai_combat_style_names).
void ai_group_combat_think(ai_group *g)
{
    int i;

    for (i = 0; i < ai_think_num_members; i++) {
        Character *m = ai_think_members[i];
        if (m == cur_character)
            continue;
        if (ai_group_count_helpless(g) < g->num_enemies && !ai_member_skips_group_orders(m))
            m->ai.flags &= ~0x10;
        int style = m->ai.get_combat_style();
        if ((style == 4 || style == 5) && !m->has_status(STATUS_SILENCE) && (m->ai.flags & 0x80)
            && (float)m->ap / (float)m->get_max_stat(1) > 0.4f)
            m->ai.flags &= ~0x80;
        if (style == 3 || style == 5) {
            ai_group_think_caster(m, g, 1);
            if (m->ai.target != -1) {
                Character *t = living_from_handle(m->ai.target);
                if (t && !m->has_line_of_sight_to(t)) {
                    m->ai.target = -1;
                    m->ai.goals_remove_type1();
                }
            }
        }
    }

    if (g->num_enemies == 0) {
        g->flags &= ~AI_GROUP_FIGHTING;
        return;
    }
    for (i = 0; i < ai_think_num_members; i++) {
        Character *m = ai_think_members[i];
        if (m->type != CHAR_TYPE_4 && m->ai.get_combat_style() == 0)
            ai_group_think_style0(m, g);
    }
    for (i = 0; i < ai_think_num_members; i++) {
        Character *m = ai_think_members[i];
        if (m->type != CHAR_TYPE_4 && m->ai.get_combat_style() == 1)
            ai_group_think_style1(m, g);
    }
    for (i = 0; i < ai_think_num_members; i++) {
        Character *m = ai_think_members[i];
        if (m->type != CHAR_TYPE_4) {
            int style = m->ai.get_combat_style();
            if (style == 2 || style == 4)
                ai_group_think_style2(m, g);
        }
    }
    for (i = 0; i < ai_think_num_members; i++) {
        Character *m = ai_think_members[i];
        if (m->type != CHAR_TYPE_4) {
            int style = m->ai.get_combat_style();
            if (style == 3 || style == 5)
                ai_group_think_caster(m, g, 0);
        }
    }
}

// 0x493000  Confidence: exact. Helpless enemies (ai_think_enemies, D3400).
int ai_group_count_helpless(ai_group *g)
{
    int n = 0;
    for (int i = 0; i < g->num_enemies; i++)
        if (character_is_helpless(ai_think_enemies[i]))
            n++;
    return n;
}

// 0x493040  Confidence: exact (an int result: mov eax, 1).
int character_is_helpless(Character *c)
{
    if (c && (c->has_status(STATUS_SLEEP) || c->has_status(STATUS_PARALYZE) || c->has_status(STATUS_ICE_COFFIN)))
        return 1;
    return 0;
}

// 0x493080  Confidence: likely (the bit test is `shr eax, 8; and al, 1`, written as a shift). True for a
// player-controlled party member (AI flag 1, or Player flag 4) and for a character whose first goal
// slot has flag 0x100.
bool ai_member_skips_group_orders(Character *c)
{
    if (c->type == CHAR_TYPE_PARTY) {
        if (c->ai.flags & AI_FLAG_PLAYER_CONTROL)
            return true;
        Player *p = player_from_handle(c->handle);
        if (p && (p->flags & 4))
            return true;
    }
    return (c->ai.goals[0].flags >> 8) & 1;
}

// 0x4930d0  Confidence: exact. Melee: the nearest enemy that is not helpless.
void ai_group_think_style0(Character *c, ai_group *g)
{
    if (ai_member_skips_group_orders(c) == true)
        return;
    if (c->ai.target != -1 && living_from_handle(c->ai.target))
        return;
    if (c->has_status(STATUS_INVISIBLE))
        return;
    if (c->has_status(STATUS_SLEEP))
        return;
    if (c->flags & CF_SUMMONLIKE)
        return;
    if (c->ai.is_mode(AI_STATE_DISABLED, -1))
        return;

    if (g->num_enemies == 1) {
        ai_group_order_attack(c, ai_think_enemies[0], g, true);
        return;
    }
    Character *best = NULL;
    float best_dist = -1.0f;
    for (int i = 0; i < g->num_enemies; i++) {
        Character *e = ai_think_enemies[i];
        if (e && !character_is_helpless(e)) {
            float d = distance(&c->pos, &e->pos);
            if (best == NULL || d < best_dist) {
                best_dist = d;
                best = e;
            }
        }
    }
    if (best) {
        ai_group_order_attack(c, best, g, false);
        return;
    }
    if (ai_think_enemies[0])
        ai_group_order_attack(c, ai_think_enemies[0], g, true);
}

// 0x493220  Confidence: exact. The first order of a party group's think also switches the party to
// combat mode.
void ai_group_order_attack(Character *c, Character *target, ai_group *g, bool mark)
{
    if ((g->flags & AI_GROUP_PARTY) && !(c->flags & CF_COMBAT_READY) && !(g->flags & AI_GROUP_COMBAT_SET)) {
        combat_mode_set(true);
        g->flags |= AI_GROUP_COMBAT_SET;
    }
    c->ai.add_attack(target, 1000);
    c->ai.process_goals();
    if (mark)
        c->ai.flags |= 0x10;
}

// 0x493290  Confidence: likely (the three tail tests are read as three if statements; see NOTES.md).
// Support: protect a member of style 2-5 that an enemy targets, else the nearest unengaged enemy, the
// enemy of the most hurt member, an engaged one, the first one. A successful order is followed by the
// fallback order on the first enemy (no return; NOTES.md).
void ai_group_think_style1(Character *c, ai_group *g)
{
    int i;

    if (ai_member_skips_group_orders(c) == true)
        return;
    if (c->has_status(STATUS_INVISIBLE))
        return;
    if (c->has_status(STATUS_SLEEP))
        return;
    if (c->flags & CF_SUMMONLIKE)
        return;
    if (c->ai.is_mode(AI_STATE_DISABLED, -1))
        return;

    if (g->num_enemies == 1) {
        ai_group_order_attack(c, ai_think_enemies[0], g, true);
        return;
    }
    for (i = 0; i < g->num_enemies; i++) {
        Character *e = ai_think_enemies[i];
        if (e && e->ai.target != -1) {
            Character *t = living_from_handle(e->ai.target);
            if (t && !character_is_helpless(t)) {
                int style = t->ai.get_combat_style();
                if (t->ai.group == g && (style == 3 || style == 2 || style == 4 || style == 5)) {
                    ai_group_order_attack(c, e, g, false);
                    c->ai.flags |= 0x10;
                    return;
                }
            }
        }
    }

    Character *best = NULL;
    Character *engaged = NULL;
    Character *first = NULL;
    float best_dist = -1.0f;
    if (c->ai.target != -1 && living_from_handle(c->ai.target))
        return;
    for (i = 0; i < g->num_enemies; i++) {
        Character *e = ai_think_enemies[i];
        if (e == NULL)
            return;
        if (character_is_helpless(e))
            continue;
        if (first == NULL)
            first = e;
        int n = e->ai.count_engaged_attackers(false);
        if (n > 0) {
            if (n < 10)
                engaged = e;
            continue;
        }
        float d = distance(&c->pos, &e->pos);
        if (best == NULL || d < best_dist) {
            best_dist = d;
            best = e;
        }
    }
    float lowest = 1.0f;
    if (best == NULL) {
        for (i = 0; i < ai_think_num_members; i++) {
            Character *m = ai_think_members[i];
            if (m->ai.target != -1) {
                float frac = m->hp / (float)m->get_max_stat(0);
                if (frac < lowest) {
                    lowest = frac;
                    best = living_from_handle(m->ai.target);
                }
            }
        }
    }
    if (best == NULL || character_is_helpless(best))
        best = engaged;
    if (best && character_is_helpless(best))
        best = first;
    if (best)
        ai_group_order_attack(c, best, g, false);
    if (ai_think_enemies[0])
        ai_group_order_attack(c, ai_think_enemies[0], g, true);
}

// 0x493570  Confidence: likely (the found-in-loop exit as a break with a NULL-initialised result).
// Range / caster: the first enemy someone already fights, else the first enemy.
void ai_group_think_style2(Character *c, ai_group *g)
{
    if (ai_member_skips_group_orders(c) == true)
        return;
    if (c->ai.target != -1 && living_from_handle(c->ai.target))
        return;
    if (c->has_status(STATUS_INVISIBLE))
        return;
    if (c->has_status(STATUS_SLEEP))
        return;
    if (c->flags & CF_SUMMONLIKE)
        return;
    if (c->ai.is_mode(AI_STATE_DISABLED, -1))
        return;

    if (g->num_enemies == 1) {
        ai_group_order_attack(c, ai_think_enemies[0], g, true);
        return;
    }
    Character *first = NULL;
    Character *target = NULL;
    for (int i = 0; i < g->num_enemies; i++) {
        Character *e = ai_think_enemies[i];
        if (e == NULL)
            return;
        if (character_is_helpless(e))
            return;
        if (first == NULL)
            first = e;
        if (e->ai.count_engaged_attackers(false) > 0) {
            target = e;
            break;
        }
    }
    if (target) {
        ai_group_order_attack(c, target, g, false);
        return;
    }
    if (first) {
        ai_group_order_attack(c, first, g, false);
        return;
    }
    if (ai_think_enemies[0])
        ai_group_order_attack(c, ai_think_enemies[0], g, true);
}

// 0x4936c0  Confidence: likely (the candidate count is incremented twice per scored member, as in the
// binary; the candidate records are boss_target (0x10 bytes, five on the stack). Healer / caster: with
// more than a quarter of its AP a support spell (ai_choose_support_spell) on the members; on the second
// pass, when the enemies' attack rating is more than three times the group's, spell 0x15 or 0x13 on
// the members that are not blessed (weighted by attackers and lost HP); else the melee (style 3) or
// range pass.
void ai_group_think_caster(Character *c, ai_group *g, int first_pass)
{
    boss_target list[5];
    boss_target *chosen;
    int i, count;

    if (ai_member_skips_group_orders(c) == true)
        return;
    if (c->has_status(STATUS_INVISIBLE))
        return;
    if (c->has_status(STATUS_SLEEP))
        return;
    if (c->flags & CF_SUMMONLIKE)
        return;
    ai_state *ai = &c->ai;
    if (ai->is_casting())
        return;
    if (ai->is_mode(AI_STATE_DISABLED, -1))
        return;
    if (ai->count_attackers() > 0)
        return;

    int can_group = c->can_cast_spell(SPELL_GROUP_SUPPORT);
    int can_member = c->can_cast_spell(SPELL_MEMBER_SUPPORT);
    float ap = (float)c->ap;
    bool has_ap = ap / (float)c->get_max_stat(1) > 0.25f;
    count = 0;
    for (i = 0; i < ai_think_num_members; i++) {
        list[i].character = ai_think_members[i];
        list[i].score = 0;
        list[i].flags = 0;
    }

    if (has_ap) {
        chosen = NULL;
        int spell = ai->choose_support_spell(list, ai_think_num_members, &chosen);
        if (spell != -1) {
            if (chosen == NULL)
                ai->add_cast_spell(spell, 999999);
            else
                ai->add_cast_at_target(spell, chosen->character, 999999);
            ai->process_goals();
            return;
        }
        if (first_pass != 0)
            return;

        float own = 0.0f, foe = 0.0f;
        for (i = 0; i < ai_think_num_members; i++)
            own += ai_think_members[i]->combat.attack_rating();
        for (i = 0; i < g->num_enemies; i++)
            foe += ai_think_enemies[i]->combat.attack_rating();
        if (foe / own > 3.0f) {
            float total = 0.0f;
            for (i = 0; i < ai_think_num_members; i++) {
                Character *m = ai_think_members[i];
                list[i].flags = 0;
                list[i].score = 0;
                if (can_member && !m->has_status(STATUS_BLESS)) {
                    float frac = m->hp / (float)m->get_max_stat(0);
                    int n = m->ai.count_engaged_attackers(true);
                    list[i].flags |= 4;
                    count++;
                    float score = (float)n * 10.0f + (30.0f - frac * 30.0f);
                    if (score > 0.0f) {
                        total += score;
                        count++;
                        list[i].score = total;
                    }
                }
            }
            if (count > 0) {
                if (can_group && count > 2) {
                    ai->add_cast_spell(SPELL_GROUP_SUPPORT, 999999);
                    ai->process_goals();
                    return;
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
                if (pick != -1) {
                    ai->add_cast_at_target(SPELL_MEMBER_SUPPORT, ai_think_members[pick], 999999);
                    ai->process_goals();
                    return;
                }
            }
        }
    } else if (first_pass != 0) {
        return;
    }

    if (ai->get_combat_style() == 3)
        ai_group_think_style0(c, g);
    else
        ai_group_think_style2(c, g);
}

// 0x493ac0  Confidence: likely (the cone test; see NOTES.md). The group's members go to
// ai_think_members and every active enemy of the leader that a member sees (within its fov range and
// line of sight: inside the detection range, or, when not blind, at a dot product below 0.71 with its
// forward vector) and that is within 3 m of the leader's height goes to the group's enemy list (10
// max) and ai_think_enemies (D3400).
void ai_group_collect_enemies(ai_group *g)
{
    Character *leader = living_from_handle(g->leader);
    int i;

    if (leader == NULL)
        return;
    ai_think_num_members = 0;
    for (i = 0; i < g->count; i++) {
        ai_think_members[ai_think_num_members] = living_from_handle(g->members[i]);
        if (ai_think_members[ai_think_num_members])
            ai_think_num_members++;
    }
    g->num_enemies = 0;
    for (Character *e = living_list.live_next; e != &living_list; e = e->live_next) {
        if (!leader->is_enemy(e))
            continue;
        if (e->flags & 0x20000)
            continue;
        if (!(e->obj_flags & 1))
            continue;
        if ((game_flags & GF_SINGLE_PLAYER) && (e->ai.flags & 0x40))
            continue;
        for (i = 0; i < ai_think_num_members; i++) {
            Character *m = ai_think_members[i];
            float d = distance(&e->pos, &m->pos);
            if (d <= m->info->fov_range && m->has_line_of_sight_to(e)) {
                if (d < m->info->detection_range)
                    break;
                if (m->has_status(STATUS_BLIND) != true) {
                    vector3 dir = e->pos - m->pos;
                    dir.normalize();
                    if (dir.y * m->orient.fvec.y + dir.z * m->orient.fvec.z + dir.x * m->orient.fvec.x < 0.71f)
                        break;
                }
            }
        }
        if (i >= ai_think_num_members)
            continue;
        if (fabs(cur_character->pos.y - e->pos.y) <= 3.0f && g->num_enemies < 10) {
            g->enemies[g->num_enemies] = e->handle;
            ai_think_enemies[g->num_enemies] = e;
            g->num_enemies++;
        }
    }
    ai_group_shuffle_enemies(g);
}

// 0x493ce0  Confidence: exact. 2n random swaps, then the pointers again.
void ai_group_shuffle_enemies(ai_group *g)
{
    int i;

    if (g->num_enemies <= 1)
        return;
    for (i = 0; i < g->num_enemies * 2; i++) {
        int a = rand() % g->num_enemies;
        int b = rand() % g->num_enemies;
        int t = g->enemies[a];
        g->enemies[a] = g->enemies[b];
        g->enemies[b] = t;
    }
    for (i = 0; i < g->num_enemies; i++)
        ai_think_enemies[i] = living_from_handle(g->enemies[i]);
}

// ---- console command -------------------------------------------------------------------------------
// 0x493d60 init_ccmd_party_controller_check_thunk (jmp, static initialiser entry 264). Confidence: exact.
// 0x493d70 init_ccmd_party_controller_check (console_command constructor). Confidence: exact.
console_command ccmd_party_controller_check("party_controller_check", "Toggles Party_controller_check",
                                            cmd_party_controller_check);   // 0x243e388

// 0x493d90  Confidence: exact (the console's bool command macro; no code reads the variable).
void cmd_party_controller_check()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Party_controller_check = true;
        else if (arg_type & ARG_FALSE)
            Party_controller_check = false;
        else if (arg_type & ARG_NONE)
            Party_controller_check ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "party_controller_check", "Party_controller_check");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "party_controller_check", Party_controller_check ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}
