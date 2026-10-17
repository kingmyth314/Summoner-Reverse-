// unnamed_level_complete_0x4b0960.cpp - Sum.exe v1.40, code 0x4b0960-0x4b140f: the multiplayer level and
// difficulty completion (packets 0x3c L_CMPLT, 0x3d D_CMPLT, the level boss watch) and the combat exchange
// packet 0x22 COM_SEQ (file name not known: a placeholder, see src/decomp/README.md).
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump table 0x4b10e4 read from Sum.exe).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: after multispell.cpp's mp_summon_claim_release (0x4b0940; multispell.cpp is tagged by
// strings.tbl id 238 and has no .data), to mp_handle_combat_sequence's RET (0x4b1402); the next unit opens
// with its initialiser run at 0x4b1410 (entries 303 / 304). It may be the tail of multispell.cpp: its .bss
// 0x247e830 follows multispell's 0x247e824-0x247e82f, but that is only adjacency (NOTES).
// Data: .data 0x59d694-0x59d6e3: the three boss names not pooled with other units ("Red Oni", "Galienne#
// $kingroom", "Barbarian Fighter#boss": the order of use reversed, as VC6 lays out one function's
// literals) and "resetting %s to attack %s\n"; one block, so the COM_SEQ functions belong with the level
// code. The other eleven boss names are pooled with earlier units (0x596648-0x599940).
// .bss 0x247e830-0x247e83b.

#include <stdio.h>
#include <string.h>
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/ai/ai.h"
#include "levelscripts/level_scripts.h"
#include "summoner/summoner.h"
#include "summoner/multi/multi.h"

// ---- .bss ----------------------------------------------------------------------------------------
bool mp_level_done;                                  // 0x247e830
int  mp_level_index;                                 // 0x247e834
Character *mp_level_boss;                            // 0x247e838

// 0x4b0960  Confidence: exact.
void mp_send_level_complete(int level)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!MP_IS_HOST())
        return;
    MP_BUILD_HEADER(MP_L_CMPLT);
    MP_ADD_DATA(level);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x4b09c0  Confidence: exact.
void mp_handle_level_complete(ubyte *data, void *from)
{
    if (MP_IS_CLIENT())
        mp_mark_level_complete(*(int *)data);
}

// 0x4b09f0  Confidence: exact.
void mp_mark_level_complete(int level)
{
    mp_player *p;

    if (level < 0 || level >= MP_NUM_LEVELS)
        return;
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next)
        p->profile_flags |= 1 << level;
    if (MP_IS_HOST())
        mp_send_level_complete(level);
}

// 0x4b0a50  Confidence: exact.
void mp_send_difficulty_complete(int difficulty)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!MP_IS_HOST())
        return;
    MP_BUILD_HEADER(MP_D_CMPLT);
    MP_ADD_DATA(difficulty);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, true);
}

// 0x4b0ab0  Confidence: exact.
void mp_handle_difficulty_complete(ubyte *data, void *from)
{
    if (MP_IS_CLIENT())
        mp_mark_difficulty_complete(*(int *)data);
}

// 0x4b0ae0  Confidence: exact. The flags word is replaced by the difficulty bits, not or-ed with them
// (`mov [eax+0x30], edx`): the level bits 0..25 are cleared on every player that lacks one of the bits
// (game bug or a deliberate reset of the level progress, NOTES).
void mp_mark_difficulty_complete(int difficulty)
{
    mp_player *p;
    unsigned int mask;
    int i;

    if (difficulty < 0 || difficulty >= 3)
        return;
    mask = 0;
    for (i = 0; i <= difficulty; i++)
        mask |= 1 << (i + MP_DIFFICULTY_BIT);
    for (p = mp_player_list.next; p != &mp_player_list; p = p->next)
        if ((p->profile_flags & mask) != mask)
            p->profile_flags = mask;
    if (MP_IS_HOST())
        mp_send_difficulty_complete(difficulty);
}

// 0x4b0b50  Confidence: likely (jump table 0x4b10e4 over the 26 level_table indices; each boss case has
// its own living-list loop with an inline strcmp, the found case is tail-merged). The levels without a
// boss are completed at once.
void mp_level_find_boss()
{
    Character *c;
    int i;

    mp_level_done = false;
    mp_level_index = -1;
    mp_level_boss = NULL;
    for (i = 0; i < MP_NUM_LEVELS; i++)
        if (cur_level_index == level_table[i].id)
            break;
    if (i == MP_NUM_LEVELS)
        return;
    mp_level_index = i;
    switch (i) {
    case 0:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Barbarian Fighter#boss")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 4:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Galienne#$kingroom")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 6:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Carados")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 8:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Ikaemos Priest King")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 10:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Red Oni")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 13:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Sornehan")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 14:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Sand Minotaur")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 16:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Pijian")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 17:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Guardian of the Shrine")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 18:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Murod")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 19:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Pyrul")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 21:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Titus")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 22:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Azha the Archlich")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 25:
        for (c = living_list.live_next; c != &living_list; c = c->live_next)
            if (!strcmp(c->name, "Machival")) {
                mp_level_boss = c;
                return;
            }
        break;
    case 1: case 2: case 3: case 5: case 7: case 9: case 11: case 12: case 15: case 20: case 23: case 24:
        mp_mark_level_complete(i);
        mp_level_done = true;
        break;
    }
}

// 0x4b1150  Confidence: exact. A boss that is not found leaves mp_level_boss NULL, which counts as dead.
void mp_level_complete_check()
{
    if (mp_level_done)
        return;
    if (mp_level_boss && !(mp_level_boss->flags & CF_DEAD))
        return;
    mp_mark_level_complete(mp_level_index);
    mp_level_done = true;
    if (mp_level_index == MP_FINAL_LEVEL)
        mp_mark_difficulty_complete(mp_game_difficulty);
}

// 0x4b11a0  Confidence: exact (the send is buffered: the host to everyone, a client to the host).
void mp_send_living_combat(int attacker, int defender, ubyte variant, move_info *defence, ubyte hit,
                           ubyte skill, bool special)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_COM_SEQ);
    MP_ADD_DATA(hit);
    MP_ADD_DATA(variant);
    MP_ADD_DATA(skill);
    MP_ADD_DATA(attacker);
    MP_ADD_DATA(defender);
    MP_ADD_DATA(special);
    if (!hit) {
        char move = defence ? (char)defence->category : -1;
        MP_ADD_DATA(move);
    }
    MP_END_PACKET();
    if (MP_IS_HOST())
        mp_broadcast_packet_buffered(data, packet_size, false);
    else
        multi_send_packet_buffered(data, packet_size, NULL, false);
}

// 0x4b1280  Confidence: likely (the byte fields are copied to locals first). A host whose party member's
// target was attacking it already drops that target (ai_go_idle) before the exchange is replayed.
void mp_handle_combat_sequence(ubyte *data, void *from)
{
    ubyte hit = data[0];
    ubyte variant = data[1];
    ubyte skill = data[2];
    int attacker_net = *(int *)(data + 3);
    int defender_net = *(int *)(data + 7);
    bool special = data[0xb];
    Character *a = (Character *)mp_obj_from_net_handle(attacker_net);
    Character *t = (Character *)mp_obj_from_net_handle(defender_net);
    char defence;

    if (!a || !t)
        return;
    if (skill == SKILL_BACKSTAB)
        a->ai.net_attack_target(t);
    else if (special)
        a->ai.attack_target_sub10(t);
    else
        a->ai.attack_target(t);
    defence = -1;
    if (!hit) {
        defence = data[0xc];
        t->ai.attack_target(a);
    } else if (MP_IS_HOST() && a->type == 7) {
        ai_state *ai = &t->ai;
        if (ai->is_mode(AI_STATE_COMBAT, -1) && t->ai.target == a->handle) {
            printf("resetting %s to attack %s\n", t->name, a->name);
            ai->go_idle();
        }
    }
    a->combat.apply_net_exchange(hit, variant, defence, skill, special != 0);
    if (MP_IS_HOST())
        mp_forward_cur_packet_buffered(from);
}
