// unnamed_goals_doors_0x475180.cpp - Sum.exe v1.40, code 0x475180-0x4778ab: the multiplayer handle and
// player-list helpers, the AI goal slots (evaluate, sort, find, alloc, add, remove, dispatch), the AI
// service request (SRV_REQ), the doors and the level object / level record lookups.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs, llvm-objdump). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 /Ob1. Confidence per function: exact (control flow, calls, constants and globals match), likely (same,
// but one detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range (decomp58, NOTES.md, D3800): ai.cpp ends with cast_effect_track_count (ret 0x475178, D1905).
// Its .data ends with its literals ("Ai_path_wandering_enabled" ends at 0x59b7c6); this unit's .data
// starts with an initialised variable, ai_goal_next_id (0x59b7c8 = 1), so a new object starts there, and
// its only literal, "WARNING: unknown service request type -- %d\n" (0x59b7cc-0x59b7f9), follows. The
// camera unit's .data starts with its own variables at 0x59b7fc and its code with the static initialiser
// of ccmd_zoom_limit (0x4778b0, static_initializers entry 229). No .bss. Its .rdata float constants
// 0x581a0c-0x581a24 (mp_quantize_position .. door_start_move) follow ai.cpp's (0x5819fc-0x581a08).
// Whether the mp helpers (0x475180-0x475823), the goals (0x475830-0x4768a0) and the doors / level objects
// (0x476890-0x4778ab) are one object or several cannot be told: none of them has data of its own except
// ai_goal_next_id and the SRV_REQ literal. Kept as one placeholder.
// Data: .data 0x59b7c8-0x59b7fb.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/net/net.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multicontainer.h"

#define LOBJ_FLAG_TEXTURED  0x20          // a waving object with a textured record (lobj_find_textured_record)
#define LOBJ_FLAG_HIDDEN    0x80          // lobjs_hide_by_prefix / lobjs_show_by_prefix
#define LOBJ_FLAG_DOOR      0x100

// ---- .data (0x59b7c8) -------------------------------------------------------------------------------
int ai_goal_next_id = 1;              // 0x59b7c8 (saved in the LENT chunk by level_scripts.cpp)

// ---- multiplayer helpers -----------------------------------------------------------------------------
// 0x475180  Confidence: exact. nw_addr_equal is 0 for a match (D5903: the source tested it the wrong way).
mp_player *mp_player_find_by_addr(const void *addr, bool check_port)
{
    mp_player *p;

    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (nw_addr_equal(p->addr, addr, check_port) == 0)
            return p;
    }
    return NULL;
}

// 0x4751d0  Confidence: exact.
mp_player *mp_player_find_by_id(int id)
{
    mp_player *p;

    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (p->id == id)
            return p;
    }
    return NULL;
}

// 0x475200  Confidence: exact.
mp_player *mp_player_find_by_name(const char *name)
{
    mp_player *p;

    if (name) {
        for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
            if (_stricmp(p->name, name) == 0)
                return p;
        }
    }
    return NULL;
}

// 0x475240  Confidence: exact. A client maps the host's handle through mp_net_handle_map; the host's
// handles are the net handles.
obj *mp_obj_from_net_handle(int net_handle)
{
    if (net_handle == -1)
        return NULL;
    if (!MP_IS_HOST())
        net_handle = mp_net_handle_map[handle_to_index(net_handle)];
    return obj_from_handle(net_handle);
}

// 0x475280  Confidence: exact (`or eax, eax` returns the -1 itself).
int mp_local_handle_from_net(int net_handle)
{
    if (net_handle == -1)
        return net_handle;
    if (!MP_IS_HOST())
        net_handle = mp_net_handle_map[handle_to_index(net_handle)];
    return net_handle;
}

// 0x4752c0  Confidence: exact.
int mp_handle_to_net(int handle)
{
    obj *o;

    if (MP_IS_HOST())
        return handle;
    o = obj_from_handle(handle);
    if (o)
        return o->net_handle;
    return -1;
}

// 0x475300  Confidence: exact. The object gets the net handle as well.
void mp_map_net_handle(int net_handle, int handle)
{
    int index;
    obj *o;

    if (net_handle < 0 || handle < 0)
        return;
    index = handle_to_index(net_handle);
    if (index < 0 || index >= MP_NET_HANDLE_MAP_SIZE)
        return;
    mp_net_handle_map[index] = handle;
    o = obj_from_handle(handle);
    if (o)
        o->net_handle = net_handle;
}

// 0x475350  Confidence: exact.
bool mp_all_players_have_status(int status)
{
    mp_player *p;

    for (p = mp_player_list.next; p != &mp_player_list; p = p->next) {
        if (p->type != status)
            return false;
    }
    return true;
}

// 0x475380  Confidence: likely (the two pairs go through stack temporaries: two vector2-like copies).
// GAME BUG (NOTES): the second pair should be uvec.x / uvec.y, but rvec.x / rvec.y is stored again, so
// every NEW_LEP orientation arrives with the up row's first two terms replaced by the right row's.
void matrix3_pack_orientation(mp_packed_orient *out, const matrix *m)
{
    out->signs = 0;
    if (m->rvec.z < 0.0f)
        out->signs = 1;
    if (m->uvec.z < 0.0f)
        out->signs |= 2;
    if (m->fvec.z < 0.0f)
        out->signs |= 4;
    if (m->fvec.x < 0.0f)
        out->signs |= 8;
    if (m->fvec.y < 0.0f)
        out->signs |= 0x10;
    vector2 a = *(const vector2 *)&m->rvec;
    vector2 b = *(const vector2 *)&m->rvec;
    out->r0 = a.x;
    out->r1 = a.y;
    out->u0 = b.x;
    out->u1 = b.y;
}

// 0x475430  Confidence: likely (the x87 schedule keeps 1 - r0^2 for m[6]; the order of the five square
// roots is the code's). The dropped terms are rebuilt as unit-length rows and columns, then signed.
void matrix3_unpack_orientation(const mp_packed_orient *in, matrix *out)
{
    float r0 = in->r0, r1 = in->r1, u0 = in->u0, u1 = in->u1;
    float a, rz, uz, fz, fx, fy;

    out->rvec.x = r0;
    out->rvec.y = r1;
    out->uvec.x = u0;
    out->uvec.y = u1;
    a = 1.0f - r0 * r0;
    rz = (float)sqrt(fabsf(a - r1 * r1));
    out->rvec.z = rz;
    uz = (float)sqrt(fabsf(1.0f - u0 * u0 - u1 * u1));
    out->uvec.z = uz;
    fz = (float)sqrt(fabsf(1.0f - rz * rz - uz * uz));
    out->fvec.z = fz;
    fx = (float)sqrt(fabsf(a - u0 * u0));
    out->fvec.x = fx;
    fy = (float)sqrt(fabsf(1.0f - r1 * r1 - u1 * u1));
    out->fvec.y = fy;
    out->rvec.z = rz * ((in->signs & 1) ? -1.0f : 1.0f);
    out->uvec.z = uz * ((in->signs & 2) ? -1.0f : 1.0f);
    out->fvec.z = fz * ((in->signs & 4) ? -1.0f : 1.0f);
    out->fvec.x = fx * ((in->signs & 8) ? -1.0f : 1.0f);
    out->fvec.y = fy * ((in->signs & 0x10) ? -1.0f : 1.0f);
}

// 0x4755d0  Confidence: exact. NEW_MON: an empty packet to everybody.
void mp_broadcast_packet_3e()
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    MP_BUILD_HEADER(MP_NEW_MON);
    MP_END_PACKET();
    mp_broadcast_packet(data, packet_size, 1);
}

// 0x475610  Confidence: exact. NEW_MON: the local NPCs and monsters (types 4, 5, 9) are deleted before the
// host sends its own.
void mp_handle_new_mon(ubyte *data, void *from)
{
    Character *c, *next;

    for (c = living_list.live_next; c != &living_list; c = next) {
        next = c->live_next;
        if (c->type >= 4 && (c->type <= 5 || c->type == 9))
            delete c;
    }
}

// 0x475660  Confidence: exact (VC6 byte match) (the z range is computed first and kept on the x87 stack).
void mp_quantize_position(short *x, short *y, short *z, vector3 pos)
{
    float dz = pf_bounds_max_z - pf_bounds_min_z;
    float fy;

    *x = (short)((pos.x - pf_bounds_min_x) / (pf_bounds_max_x - pf_bounds_min_x) * 65535.0f);
    fy = (pos.y - -1024.0f) * (1.0f / 2048.0f);
    *y = (short)(fy * 65535.0f);
    *z = (short)((pos.z - pf_bounds_min_z) / dz * 65535.0f);
}

// 0x4756e0  Confidence: exact (VC6 byte match under 8966: a local filled field by field; the y constant 2048 / 65535 is folded).
vector3 mp_dequantize_position(ushort x, ushort y, ushort z)
{
    vector3 v;
    float fx = x, fz = z;
    v.x = (pf_bounds_max_x - pf_bounds_min_x) * fx * (1.0f / 65535.0f) + pf_bounds_min_x;
    v.y = y * (2048.0f / 65535.0f) - 1024.0f;
    v.z = (pf_bounds_max_z - pf_bounds_min_z) * fz * (1.0f / 65535.0f) + pf_bounds_min_z;
    return v;
}

// 0x475780  Confidence: exact (jump table 0x47580c over types 4..9). Single player: every NPC, monster
// and party member. Multiplayer: the own character, the party members of the local player, and on the
// host the NPCs and monsters.
bool character_is_locally_controlled(Character *c)
{
    if (!(game_flags & GF_MULTIPLAYER)) {
        switch (c->type) {
        case 4:
        case 5:
        case 7:
        case 9:
            return true;
        }
        return false;
    }
    if (c == cur_character)
        return true;
    if (c->type == 7) {
        Player *p = player_from_handle(c->handle);
        if (p && p->mp_owner == mp_local_player)
            return true;
    }
    if (MP_IS_HOST() && c->type >= 4 && (c->type <= 5 || c->type == 9))
        return true;
    return false;
}

// ---- goals -----------------------------------------------------------------------------------------
// 0x475830  Confidence: likely (jump table 0x475cd4 over types 1..17; the returns all go through EBX,
// so `result` is a variable; cases 10, 11, 13 and 17 set it to 0 explicitly). cdecl (ai, goal).
// GAME BUG (NOTES): a cast-at-target goal (4) whose target is gone reads the NULL target's type when a
// cutscene script runs.
int ai_goal_evaluate(ai_state *ai, ai_goal *goal)
{
    int result;
    Character *t;
    vector3 dest;

    if (ai->is_mode(AI_STATE_DISABLED, -1))
        return AI_GOAL_SUSPEND;
    result = AI_GOAL_KEEP;
    switch (goal->type) {
    case AI_GOAL_ATTACK:
        if (ai->is_casting() == true)
            return AI_GOAL_SUSPEND;
        if (ai->owner->flags & CF_COMBAT_READY) {
            t = living_from_handle(goal->target);
            if (!t)
                return AI_GOAL_LOST;
            if ((t->flags & CF_HIDDEN) || (cutscene_script_is_active() && t->type == 7))
                result = AI_GOAL_SUSPEND;
            if (t->flags & CF_DEAD)
                result = AI_GOAL_DONE;
            if (ai->owner->get_team() == 0 && t->get_team() == 0 && !t->has_status(0x10))
                return AI_GOAL_DONE;
        }
        break;
    case AI_GOAL_FOLLOW:
    case AI_GOAL_PROTECT:
        t = living_from_handle(goal->target);
        if (!t)
            return AI_GOAL_LOST;
        if (t->is_dead_type())
            result = AI_GOAL_DONE;
        if (goal->type == AI_GOAL_FOLLOW) {
            Character *o = ai->owner;
            if (o->type == 7) {
                if (player_from_handle(o->handle)->flags & PF_HOLD)
                    return AI_GOAL_SUSPEND;
                if (cutscene_script_is_active())
                    return AI_GOAL_SUSPEND;
            }
        }
        break;
    case AI_GOAL_CAST_SPELL:
    case AI_GOAL_CAST_TARGETED:
    case AI_GOAL_CAST_LOCATION:
        if (goal->id == ai->cur_goal) {
            if (!ai->is_mode(AI_STATE_CAST_SPELL, -1))
                return AI_GOAL_DONE;
            break;
        }
        if (goal->type == AI_GOAL_CAST_TARGETED) {
            t = living_from_handle(goal->target);
            if (!t)
                result = AI_GOAL_LOST;
            if (cutscene_script_is_active() && t->type == 7)
                return AI_GOAL_SUSPEND;
        }
        break;
    case AI_GOAL_SWITCH_COMBAT:
        if (goal->id == ai->cur_goal) {
            unsigned int f = ai->owner->flags;
            if ((f & CF_COMBAT_READY) && (f & 0x400))
                return AI_GOAL_SUSPEND;
            if (!ai->is_combat_e_or_sneak_attack())
                return AI_GOAL_DONE;
            break;
        }
        if (ai->is_casting() == true)
            return AI_GOAL_SUSPEND;
        if (ai->is_mode(AI_STATE_SNEAKING, -1))
            return AI_GOAL_SUSPEND;
        break;
    case AI_GOAL_PATHFIND:
        if (ai->owner->is_rider()) {
            Character *mount = ai->owner->get_mount();
            if (!mount || goal->id != ai->cur_goal)
                break;
            if (!mount->ai.path_get_destination(&dest))
                return AI_GOAL_DONE;
            break;
        }
        if (goal->id != ai->cur_goal)
            break;
        if (!ai->path_get_destination(&dest))
            return AI_GOAL_DONE;
        break;
    case AI_GOAL_GOTO_NAVPOINT:
        if (goal->id != ai->cur_goal)
            break;
        if (ai->path_get_destination(&dest))
            break;
        // The path is gone: the owner is put on the point.
        ai->owner->pos = goal->pos;
        ai->owner->layer = goal->layer;
        ai->owner->set_6ac(0);
        ai->owner->set_locomotion_speed(0);
        ai->owner->find_ground();
        return AI_GOAL_DONE;
    case AI_GOAL_BACKSTAB:
        t = living_from_handle(goal->target);
        if (!t)
            return AI_GOAL_LOST;
        if (ai->owner->flags & 0x400)
            break;
        if (goal->id == ai->cur_goal) {
            if (!ai->is_mode(AI_STATE_COMBAT, 0x11))
                return AI_GOAL_DONE;
            break;
        }
        if (!ai->can_backstab_target(t))
            return AI_GOAL_LOST;
        if (ai->owner->forward_speed > 0.0f || ai->is_combat_e_or_sneak_attack() == true
            || ai->is_casting() == true)
            return AI_GOAL_SUSPEND;
        break;
    case AI_GOAL_SNEAK:
        if (goal->id == ai->cur_goal) {
            if (ai->is_mode(AI_STATE_SNEAKING, -1))
                break;
            return (ai->owner->flags & CF_COMBAT_READY) ? AI_GOAL_DONE : AI_GOAL_SUSPEND;
        }
        if (ai->owner->forward_speed > 0.0f || ai->is_casting() == true
            || ai->is_combat_e_or_sneak_attack())
            return AI_GOAL_SUSPEND;
        break;
    case AI_GOAL_HIDE:
        if (goal->id == ai->cur_goal) {
            if (ai->is_mode(AI_STATE_HIDING, -1))
                break;
            if (ai->owner->flags & CF_COMBAT_READY)
                break;
            return AI_GOAL_SUSPEND;
        }
        if (ai->owner->forward_speed > 0.0f || ai->is_casting() == true)
            return AI_GOAL_SUSPEND;
        break;
    case AI_GOAL_TURN_TO:
    case AI_GOAL_RUN_AWAY:
    case AI_GOAL_WANDER:
    case AI_GOAL_PICKUP_ITEM:
        result = AI_GOAL_KEEP;
        break;
    }
    return result;
}

// 0x475d20  Confidence: exact (VC6 byte match only with the first test wrapping the rest). Kept goals (0x40)
// and suspended goals (8) sort last; combat-mode and sneak goals first; then the higher priority, then the newer
// goal.
int ai_goal_compare(const void *pa, const void *pb)
{
    const ai_goal *a = (const ai_goal *)pa;
    const ai_goal *b = (const ai_goal *)pb;

    if (!(a->flags & 0x40) || (b->flags & 0x40)) {
        if (!(a->flags & 0x40) && (b->flags & 0x40))
            return -1;
        if ((a->flags & 8) && !(b->flags & 8))
            return 1;
        if (!(a->flags & 8) && (b->flags & 8))
            return -1;
        if (a->type == AI_GOAL_SWITCH_COMBAT || a->type == AI_GOAL_SNEAK)
            return -1;
        if (b->type == AI_GOAL_SWITCH_COMBAT || b->type == AI_GOAL_SNEAK)
            return 1;
        if (a->priority > b->priority)
            return -1;
        if (a->priority < b->priority)
            return 1;
        if (a->time > b->time)
            return -1;
        return 1;
    }
    return 1;
}

// 0x475db0  Confidence: exact.
void ai_goals_sort(ai_goal *goals)
{
    qsort(goals, 5, sizeof(ai_goal), ai_goal_compare);
}

// 0x475dd0  Confidence: exact.
void ai_state::goals_remove_type9()
{
    int i;

    while (1) {
        i = goal_find(1, AI_GOAL_PATHFIND, 0, 0, -1, NULL, 0, AI_NO_SPELL);
        if (i == -1)
            return;
        goal_remove(i);
    }
}

// 0x475e00  Confidence: exact.
void ai_state::goals_remove_type1()
{
    int i;

    while (1) {
        i = goal_find(1, AI_GOAL_ATTACK, 0, 0, -1, NULL, 0, AI_NO_SPELL);
        if (i == -1)
            return;
        goal_remove(i);
    }
}

// 0x475e30  Confidence: exact.
void ai_state::goals_remove_types_3_5()
{
    int i;

    while (1) {
        i = goal_find(1, AI_GOAL_CAST_SPELL, 0, 0, -1, NULL, 0, AI_NO_SPELL);
        if (i == -1)
            break;
        goal_remove(i);
    }
    while (1) {
        i = goal_find(1, AI_GOAL_CAST_TARGETED, 0, 0, -1, NULL, 0, AI_NO_SPELL);
        if (i == -1)
            break;
        goal_remove(i);
    }
    while (1) {
        i = goal_find(1, AI_GOAL_CAST_LOCATION, 0, 0, -1, NULL, 0, AI_NO_SPELL);
        if (i == -1)
            return;
        goal_remove(i);
    }
}

// 0x475eb0  Confidence: likely (`match` lives in BL and is also the bit-0 test constant). The mask picks
// what is compared, limited to the type's fields (ai_goal_type_fields) plus type, flags and "not the
// running goal". Follow goals never compare the target. The flags compare ignores 0x30e. ret 0x20.
int ai_state::goal_find(int mask, int type, int a, int b, int c, vector3 *d, int e, int f)
{
    unsigned int flags = (unsigned int)b & ~0x30e;
    int i;

    for (i = 0; i < 5; i++) {
        ai_goal *g = &goals[i];
        bool match = true;
        unsigned int m = (ai_goal_type_fields(type) | AI_GF_TYPE | AI_GF_FLAGS | AI_GF_NOT_CUR) & mask;

        if ((m & AI_GF_TYPE) && g->type != type)
            continue;
        if ((m & AI_GF_FLAGS) && (g->flags & ~0x30e) != flags)
            continue;
        if (type != AI_GOAL_FOLLOW && (m & AI_GF_TARGET) && g->target != c)
            continue;
        if (m & AI_GF_POS) {
            if (!(g->pos == *d))
                match = false;
            if (g->layer != e || !match)
                continue;
        }
        if ((m & AI_GF_SPELL) && g->spell != f)
            continue;
        if (!match)
            continue;
        if (!(m & AI_GF_NOT_CUR) || g->id != cur_goal)
            return i;
    }
    return -1;
}

// 0x475fb0  Confidence: exact (VC6 byte match, cl 8966, match21; byte table 0x476008 into 0x475ff8).
unsigned int ai_goal_type_fields(int type)
{
    unsigned int fields = 0;

    switch (type) {
    case AI_GOAL_ATTACK:
    case AI_GOAL_FOLLOW:
    case AI_GOAL_CAST_TARGETED:
    case AI_GOAL_PROTECT:
    case AI_GOAL_BACKSTAB:
    case AI_GOAL_PICKUP_ITEM:
        fields = AI_GF_TARGET;
        break;
    case AI_GOAL_PATHFIND:
    case AI_GOAL_TURN_TO:
    case AI_GOAL_GOTO_NAVPOINT:
        fields = AI_GF_POS;
        break;
    case AI_GOAL_WANDER:
        fields = AI_GF_POS | AI_GF_ARG;
        break;
    }
    switch (type) {
    case AI_GOAL_CAST_LOCATION:
        fields |= AI_GF_POS;    // falls through: a located cast also carries the spell
    case AI_GOAL_CAST_SPELL:
    case AI_GOAL_CAST_TARGETED:
        fields |= AI_GF_SPELL;
    }
    return fields;
}

// 0x476020  Confidence: exact (VC6 byte match, match29; was likely) (the slot search keeps a pointer to the best priority). The free or
// lowest-priority slot without flag 1; a goal only replaces it with a higher priority or flag 1. The
// leader of the party gets flag 0x100 and loses its hold mode outside conversations. ret 0x1c.
// GAME BUGS (NOTES): the chosen slot is returned even when the priority was too low and nothing was
// stored, and with every slot kept (-1) the leader code writes goals[-1].flags.
int ai_state::goal_alloc(int type, int priority, unsigned int flags, int target, vector3 *pos, int layer,
                         int spell)
{
    int best = -1;
    int i;
    Player *p;

    flags |= 2;
    for (i = 0; i < 5; i++) {
        if (goals[i].flags & 1)
            continue;
        if (best < 0 || goals[i].priority < goals[best].priority)
            best = i;
    }
    if (best >= 0 && ((flags & 1) || priority > goals[best].priority)) {
        goals[best].type = type;
        goals[best].priority = priority;
        goals[best].flags = flags;
        goals[best].id = ai_goal_next_id;
        ai_goal_next_id++;
        goals[best].time = level_session_time;
        goals[best].path = NULL;
        goals[best].target = target;
        if (pos)
            goals[best].pos = *pos;
        goals[best].layer = layer;
        goals[best].spell = spell;
    }
    if (owner->type == 7) {
        p = player_from_handle(owner->handle);
        if (p && p->is_leader() == true) {
            if (!(game_flags & GF_CONVERSATION))
                p->flags &= ~PF_HOLD;
            goals[best].flags |= 0x100;
        }
    }
    return best;
}

// 0x476190  Confidence: exact. ret 0x20. An attack goal removes the other attack and backstab goals, a
// travel goal the other travel goals, a cast goal the other cast goals; a player-controlled owner also
// drops travel and attack goals for either. An equal goal only takes the new priority (and a follow goal
// the new target).
int ai_state::goal_add(int type, int priority, unsigned int flags, int target, vector3 *pos, int layer,
                       int spell, float arg)
{
    int i;
    unsigned int fields;

    if (type == AI_GOAL_ATTACK) {
        goals_remove_type1();
        while (1) {
            i = goal_find(1, AI_GOAL_BACKSTAB, 0, 0, -1, NULL, 0, AI_NO_SPELL);
            if (i == -1)
                break;
            goal_remove(i);
        }
    } else if (type == AI_GOAL_PATHFIND) {
        goals_remove_type9();
    } else if (type == AI_GOAL_CAST_SPELL || type == AI_GOAL_CAST_TARGETED || type == AI_GOAL_CAST_LOCATION) {
        goals_remove_types_3_5();
    }
    if ((this->flags & AI_FLAG_PLAYER_CONTROL) && (type == AI_GOAL_PATHFIND || type == AI_GOAL_ATTACK)) {
        goals_remove_type9();
        goals_remove_type1();
    }
    i = goal_find(0x1f, type, priority, flags, target, pos, 0, AI_NO_SPELL);
    if (i >= 0) {
        goals[i].priority = priority;
        if (type == AI_GOAL_FOLLOW)
            goals[i].target = target;
        return i;
    }
    i = goal_alloc(type, priority, flags, -1, NULL, 0, AI_NO_SPELL);
    if (i >= 0) {
        fields = ai_goal_type_fields(type);
        if (fields & AI_GF_TARGET)
            goals[i].target = target;
        if (fields & AI_GF_POS) {
            goals[i].pos = *pos;
            goals[i].layer = layer;
        }
        if (fields & AI_GF_SPELL)
            goals[i].spell = spell;
        if (fields & AI_GF_ARG)
            goals[i].arg = arg;
    }
    return i;
}

// 0x476300  Confidence: exact (VC6 byte match, cl 8966, match21). ret 4.
bool ai_state::goal_remove(int slot)
{
    if (slot < 0 || slot >= 5)
        return false;
    goals[slot].type = 0;
    goals[slot].priority = -1;
    goals[slot].flags = 0;
    ai_path_free(goals[slot].path);
    goals[slot].path = NULL;
    if (goals[slot].id == cur_goal)
        cur_goal = -1;
    return true;
}

// 0x476380  Confidence: exact.
void ai_state::goals_clear()
{
    int i;

    for (i = 0; i < 5; i++)
        goal_remove(i);
}

// 0x4763a0  Confidence: exact (jump table 0x4765c4 over types 1..17; each case returns).
void ai_state::process_goals()
{
    bool walk;

    if (is_mode(AI_STATE_SUMMONING, -1) == true)
        return;
    ai_goals_validate(this);
    ai_goals_sort(goals);
    if (goals[0].type == 0)
        return;
    if (goals[0].flags & 8)
        return;
    if (goals[0].id == cur_goal && !is_mode(AI_STATE_IDLE, -1))
        return;
    cur_goal = goals[0].id;
    if (!(owner->flags & 0x200))
        owner->set_speed_scale(owner->run_speed, false);
    switch (goals[0].type) {
    case AI_GOAL_ATTACK:
        goal_do_attack(goals[0].target, goals[0].flags);
        return;
    case AI_GOAL_FOLLOW:
        goal_do_follow(goals[0].target);
        return;
    case AI_GOAL_PROTECT:
        goal_do_approach_party(goals[0].target);
        return;
    case AI_GOAL_CAST_SPELL:
        goal_do_cast_spell(goals[0].spell);
        return;
    case AI_GOAL_CAST_TARGETED:
        goal_do_cast_at_target(goals[0].spell, goals[0].target);
        return;
    case AI_GOAL_CAST_LOCATION:
        goal_do_cast_at_point(goals[0].spell, &goals[0].pos, goals[0].layer);
        return;
    case AI_GOAL_SWITCH_COMBAT:
    case AI_GOAL_SWITCH_NORMAL:
        goal_do_combat_mode();
        return;
    case AI_GOAL_PATHFIND:
        goal_do_travel(&goals[0].pos, goals[0].layer, goals[0].priority, goals[0].flags);
        return;
    case AI_GOAL_RUN_AWAY:
        goal_do_nothing();
        return;
    case AI_GOAL_GOTO_NAVPOINT:
        walk = false;
        if (goals[0].flags & 0x10)
            walk = true;
        goal_do_move_to(&goals[0].pos, goals[0].layer, walk, goals[0].path);
        goals[0].path = NULL;
        return;
    case AI_GOAL_WANDER:
        walk = false;
        if (goals[0].flags & 0x10)
            walk = true;
        goal_do_move_to_timed(&goals[0].pos, goals[0].layer, goals[0].arg, walk);
        return;
    case AI_GOAL_HIDE:
        goal_do_hide();
        return;
    case AI_GOAL_SNEAK:
        goal_do_sneak();
        return;
    case AI_GOAL_BACKSTAB:
        goal_do_special_attack(goals[0].target);
        return;
    case AI_GOAL_PICKUP_ITEM:
        goal_do_pickup(goals[0].target);
        return;
    }
}

// 0x476610  Confidence: exact. cdecl (ai).
void ai_goals_validate(ai_state *ai)
{
    int i;

    for (i = 0; i < 5; i++) {
        ai_goal *g = &ai->goals[i];
        if (g->type) {
            int r = ai_goal_evaluate(ai, g);
            if (r == AI_GOAL_LOST || r == AI_GOAL_DONE)
                ai->goal_remove(i);
            if (r == AI_GOAL_KEEP) {
                if (g->flags & 8)
                    g->flags &= ~8;
            } else if (r == AI_GOAL_SUSPEND) {
                g->flags |= 8;
            }
        }
    }
}

// 0x476670  Confidence: exact.
bool ai_state::has_goal_without_flag20()
{
    int i;

    for (i = 0; i < 5; i++) {
        if (goals[i].type && !(goals[i].flags & 0x20))
            return true;
    }
    return false;
}

// 0x4766a0  Confidence: exact (the counter runs down: the index is not needed).
void ai_state::goals_mark_keep()
{
    int i;

    for (i = 0; i < 5; i++) {
        if (goals[i].type)
            goals[i].flags |= 0x40;
    }
}

// 0x4766c0  Confidence: exact.
void ai_state::goals_remove_unmarked()
{
    int i;

    for (i = 0; i < 5; i++) {
        if (goals[i].type && !(goals[i].flags & 0x40))
            goal_remove(i);
    }
}

// ---- service requests ------------------------------------------------------------------------------
// 0x4766f0  Confidence: exact (jump table 0x476788 over 2..7). A client sends SRV_REQ (type, own id,
// argument) to the host; types 0 / 1 (pause, unpause) carry no id. Nothing happens in single player or
// on the host (sum.symbols and ai.h said "handled locally": there is no local branch).
void ai_service_request(ai_service_req *req)
{
    ubyte data[MP_MAX_PACKET];
    ushort packet_size;

    if (!MP_IS_CLIENT())
        return;
    int type = req->type;
    MP_BUILD_HEADER(MP_SRV_REQ);
    MP_ADD_DATA(type);
    switch (type) {
    case 2:
    case 3:
    case 6:
        MP_ADD_DATA(mp_local_player->id);
        MP_ADD_DATA(req->_unk04);
        break;
    case 4:
    case 5:
    case 7:
        MP_ADD_DATA(mp_local_player->id);
        MP_ADD_DATA(req->_unk08);
        break;
    }
    MP_END_PACKET();
    multi_send_packet(data, packet_size, NULL, true);
}

// 0x4767a0  Confidence: exact (jump table 0x47686c over 0..7). SRV_REQ on the host: (type, player id,
// argument). The unknown type is only formatted into string_buffer, never printed.
void mp_handle_srv_req(ubyte *data, void *from)
{
    int type, a, b;

    if (!MP_IS_HOST())
        return;
    memcpy(&type, data, 4);
    switch (type) {
    case 0:
        game_pause(0, 1);
        return;
    case 1:
        game_unpause();
        return;
    case 2:
        memcpy(&a, data + 4, 4);
        memcpy(&b, data + 8, 4);
        container_take_gold_server(a, b);
        return;
    case 3:
        memcpy(&a, data + 4, 4);
        memcpy(&b, data + 8, 4);
        container_request_open(a, b);
        return;
    case 6:
        memcpy(&a, data + 4, 4);
        memcpy(&b, data + 8, 4);
        container_lockpick(a, b);
        return;
    case 4:
    case 5:
        memcpy(&a, data + 4, 4);
        memcpy(&b, data + 8, 4);
        if (type == 4)
            door_find_by_mesh(a, b, 1);
        else
            door_find_by_mesh(a, b, 0);
        return;
    case 7:
        memcpy(&a, data + 4, 4);
        memcpy(&b, data + 8, 4);
        door_pick_by_mesh(a, b);
        return;
    default:
        sprintf(string_buffer, "WARNING: unknown service request type -- %d\n", type);
        return;
    }
}

// ---- doors and level objects ------------------------------------------------------------------------
// 0x476890  Confidence: exact. No symbol; name ours.
int lobj_index_compare(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

// 0x4768a0  Confidence: exact.
int name_ptr_stricmp_compare(const void *a, const void *b)
{
    return _stricmp(*(char *const *)a, *(char *const *)b);
}

// 0x4768c0  Confidence: exact (the key is a whole door record: its GameTimer is constructed after the
// flag test, so it is declared there).
door_info *door_from_lobj(lobj *l)
{
    if (!(l->flags & LOBJ_FLAG_DOOR))
        return NULL;
    door_info key;
    key.lobj_index = l - lobjs;
    return (door_info *)bsearch(&key, doors, num_doors, sizeof(door_info), lobj_index_compare);
}

// 0x476940  Confidence: exact (VC6 byte match, cl 8966, match21; was likely; the easing is written out; `open` reuses the argument slot of d). While
// the door moves, the eased progress toward open (smoothstep in two halves) is compared with fraction.
bool door_is_open(door_info *d, float fraction)
{
    bool open, locked;
    float t;

    savegame_state.two_stage_get(&open, &locked, &lobjs[d->lobj_index]);
    if (d->moving) {
        t = (float)d->timer.GetElapsedTime() * 0.001f / d->duration;
        if (!d->opening)
            t = 1.0f - t;
        if (t > 0.5f)
            t = t * 4.0f - (t * t + t * t) - 1.0f;
        else
            t = t * t + t * t;
        if (t > fraction)
            return true;
    } else if (open)
        return true;
    return false;
}

// 0x4769f0  Confidence: exact.
bool lobj_door_is_open(lobj *l, float fraction)
{
    door_info *d;

    if (!lobj_is_door(l))
        return false;
    d = door_from_lobj(l);
    if (!d)
        return false;
    return door_is_open(d, fraction);
}

// 0x476a30  Confidence: exact (VC6 byte match) (the last test returns through EAX). Within 6 units horizontally; a door
// with a named path also needs the character within 0.5 of its floor point's height.
bool door_in_use_range(Character *c, lobj *door)
{
    float dx, dz;
    door_info *d;

    if (!(door->flags & LOBJ_FLAG_DOOR))
        return false;
    if (!c)
        return false;
    dx = door->pos.x - c->pos.x;
    dz = door->pos.z - c->pos.z;
    if (dz * dz + dx * dx > 36.0f)
        return false;
    d = door_from_lobj(door);
    if (d->named_path < 0)
        return true;
    return fabsf(d->floor_point().y - c->pos.y) < 0.5f;
}

// 0x476ab0  Confidence: likely (the loop exits and the ray set-up order). A door without a named path
// uses its object position. Otherwise the door's navigation edges (pf_edges with the door flag and this
// door's index, at most four) are looked up in the grid cells around the first edge's middle, for size
// classes 1-2 and the 20 layers; the middle of the first match is cast down (from 50 above, 100 long)
// against every level object on that layer, and the first hit is the floor point.
void door_find_floor_point(lobj *l)
{
    door_info *d;
    pf_edge *edges[4];
    int num_edges, door_index, size, layer, c, k, j, i, n;
    vector2 mid;
    vector3 point;

    if (!(l->flags & LOBJ_FLAG_DOOR))
        return;
    d = door_from_lobj(l);
    if (d->named_path < 0) {
        d->floor_point() = l->pos;
        return;
    }
    door_index = d - doors;
    num_edges = 0;
    for (i = pf_edge_count - 1; i >= 0; i--) {
        if ((pf_edges[i].flags & PF_EDGE_DOOR) && pf_edges[i].door == door_index)
            edges[num_edges++] = &pf_edges[i];
    }
    if (num_edges <= 0)
        return;
    for (size = 1; size < 3; size++) {
        for (layer = 0; layer < 20; layer++) {
            mid.x = (edges[0]->b->x + edges[0]->a->x) * 0.5f;
            mid.y = (edges[0]->a->y + edges[0]->b->y) * 0.5f;
            n = pf_cells_in_box(layer, size, &mid, 0.1f);
            for (c = 0; c < n; c++) {
                pf_grid_cell *cell = pf_grid_cell_results[c];
                for (k = 0; k < cell->num_walls; k++) {
                    for (j = 0; j < num_edges; j++) {
                        if (cell->walls[k] == edges[j])
                            goto found;
                    }
                }
            }
        }
    }
    return;
found:
    if (size >= 3 || layer >= 20)
        return;
    point.x = (edges[j]->b->x + edges[j]->a->x) * 0.5f;
    point.y = 0.0f;
    point.z = (edges[j]->b->y + edges[j]->a->y) * 0.5f;
    for (i = 0; i < num_lobjs; i++) {
        lobj *o = &lobjs[i];
        lobj_ray ray;
        lobj_hit hit;
        if (o->pick_layer(&point, size) != layer)
            continue;
        ray.start = point;
        ray.start.y = o->centre.y + 50.0f;
        ray.dir.x = 0.0f;
        ray.dir.y = -100.0f;
        ray.dir.z = 0.0f;
        ray._unk18 = 0.0f;
        ray.flags = 0;
        ray.require_flags = 0x40;
        ray.exclude_flags = 0;
        ray._unk28 = 0;
        ray._unk2c = 0;
        ray._unk30 = 0;
        lobj_raycast(i, &ray, &hit, true);
        if (hit.num_hits > 0) {
            d->floor_point() = ray.start + ray.dir * hit.t;
            return;
        }
    }
}

// 0x476e00  Confidence: exact. True when the door is open or already opening.
bool door_set_open(lobj *l, float close_delay, bool force, bool sound)
{
    bool open, locked;
    door_info *d;

    if (!(l->flags & LOBJ_FLAG_DOOR))
        return false;
    d = door_from_lobj(l);
    if (!d)
        return false;
    savegame_state.two_stage_get(&open, &locked, l);
    if (open || (d->moving && d->opening))
        return true;
    return door_start_move(l, close_delay, force, sound);
}

// 0x476e80  Confidence: exact (no return value: EAX is whatever is left).
void door_set_closed(lobj *l)
{
    bool open, locked;
    door_info *d;

    if (!(l->flags & LOBJ_FLAG_DOOR))
        return;
    d = door_from_lobj(l);
    if (!d)
        return;
    savegame_state.two_stage_get(&open, &locked, l);
    if (!open)
        return;
    if (d->moving && !d->opening)
        return;
    door_start_move(l, -1.0f, true, true);
}

// 0x476ee0  Confidence: exact. The level's use handler runs after a move.
bool door_toggle_state(lobj *l, bool force)
{
    bool moved;

    if (!(l->flags & LOBJ_FLAG_DOOR))
        return false;
    if (!door_from_lobj(l))
        return false;
    moved = door_start_move(l, -1.0f, force, true);
    if (moved)
        level_script_use(l->name);
    return moved;
}

// 0x476f30  Confidence: exact (VC6 byte match, match29; was likely) (the bit-field stores; `open` reuses the lobj argument slot). A locked door
// moves only when forced. A move reverses a running one from where it is (the timer gets the remaining
// part of the duration).
bool door_start_move(lobj *l, float close_delay, bool force, bool sound)
{
    bool open, locked;
    door_info *d;
    int duration, elapsed;

    d = door_from_lobj(l);
    if (!d)
        return false;
    savegame_state.two_stage_get(&open, &locked, l);
    if (!force && locked)
        return false;
    d->close_wait = 0;
    if (close_delay >= 0.0f)
        d->close_delay = 1 - (int)(close_delay * -1000.0f);
    else
        d->close_delay = 0;
    duration = (int)(d->duration * 1000.0f);
    elapsed = 0;
    savegame_state.two_stage_set(!open, locked, l);
    if (d->moving && !d->close_wait) {
        d->opening = 1 - d->opening;
        elapsed = d->timer.GetElapsedTime();
    } else {
        d->opening = !open;
        d->moving = 1;
    }
    if (sound) {
        if (d->opening) {
            if (d->open_sound.handle >= 0)
                sound_play_3d_at(d->open_sound.handle, &l->pos);
        } else {
            if (d->close_sound.handle >= 0)
                sound_play_3d_at(d->close_sound.handle, &l->pos);
        }
    }
    if (elapsed > duration)
        elapsed = duration;
    if (elapsed)
        elapsed = duration - elapsed;
    d->timer.SetTimeout(-elapsed);
    return true;
}

// 0x477080  Confidence: exact (VC6 byte match, match29; was likely) (the vector expression's temporaries; the easing as in door_is_open, with
// the direction reversed: e is the weight of the closed pose). Called for every door object each frame.
void door_update(lobj *l)
{
    bool open, locked;
    door_info *d;
    float t, e;
    quaternion q;

    if (!(l->flags & LOBJ_FLAG_DOOR))
        return;
    d = door_from_lobj(l);
    if (!d)
        return;
    if (d->close_wait && d->timer.GetElapsedTime() >= (int)d->close_delay - 1)
        door_start_move(l, -1.0f, true, true);
    savegame_state.two_stage_get(&open, &locked, l);
    if (d->moving) {
        t = (float)d->timer.GetElapsedTime() / (d->duration * 1000.0f);
        if (t >= 1.0f) {
            t = 1.0f;
            d->moving = 0;
            if (d->state & 0xfffffff0) {
                d->close_wait = 1;
                d->timer.SetTimeout(0);
            }
        }
        if (d->opening)
            t = 1.0f - t;
        if (t > 0.5f)
            e = t * 4.0f - (t * t + t * t) - 1.0f;
        else
            e = t * t + t * t;
        l->pos = d->closed_pos * e + d->open_pos * (1.0f - e);
        quaternion_slerp(&q, &d->open_orient, &d->closed_orient, e);
        q.to_matrix_b(&l->orient);
        return;
    }
    if (open) {
        l->pos = d->open_pos;
        d->open_orient.to_matrix_b(&l->orient);
    } else {
        l->pos = d->closed_pos;
        d->closed_orient.to_matrix_b(&l->orient);
    }
}

// 0x477280  Confidence: exact. bsearch of the name list (level +0x68). 0 found, -1 not.
int door_find_by_name(const char *name, lobj **out_lobj, door_info **out_door)
{
    s3d_door_name key;
    s3d_door_name *e;
    door_info *d;

    key.name = name;
    e = (s3d_door_name *)bsearch(&key, level_s3d.door_names, num_doors, sizeof(s3d_door_name),
                                 name_ptr_stricmp_compare);
    if (e && e->door >= 0 && e->door < num_doors) {
        d = &doors[e->door];
        *out_door = d;
        if (d->lobj_index >= 0 && d->lobj_index < num_lobjs) {
            *out_lobj = &lobjs[d->lobj_index];
            return 0;
        }
    }
    return -1;
}

// 0x477310  Confidence: exact.
bool lobj_is_door(lobj *l)
{
    if ((l->flags & LOBJ_FLAG_DOOR) && door_from_lobj(l)->named_path >= 0)
        return true;
    return false;
}

// 0x477340  Confidence: exact (the saved state is read for every door, before the name test).
void door_apply_saved_state(const char *name, bool open)
{
    int i;

    for (i = 0; i < num_doors; i++) {
        bool cur, locked;
        lobj *l = &lobjs[doors[i].lobj_index];
        savegame_state.two_stage_get(&cur, &locked, l);
        if (_stricmp(l->name, name) == 0 && open != cur)
            door_start_move(l, -1.0f, true, true);
    }
}

// 0x4773d0  Confidence: exact.
bool door_is_open_by_name(const char *name, float fraction)
{
    int i;

    for (i = 0; i < num_doors; i++) {
        lobj *l = &lobjs[doors[i].lobj_index];
        if (_stricmp(l->name, name) == 0) {
            door_info *d = door_from_lobj(l);
            if (!d)
                return false;
            return door_is_open(d, fraction);
        }
    }
    return false;
}

// 0x477450  Confidence: exact.
s3d_textured_record *lobj_find_textured_record(int lobj_index)
{
    s3d_textured_record key;

    if (!(lobjs[lobj_index].flags & LOBJ_FLAG_TEXTURED))
        return NULL;
    key.lobj = lobj_index;
    return (s3d_textured_record *)bsearch(&key, level_s3d.textured, level_s3d.num_textured,
                                          sizeof(s3d_textured_record), lobj_index_compare);
}

// 0x4774a0  Confidence: exact (the count and the array are read once).
bool level_marker_find(const char *name, vector3 *pos, matrix *orient)
{
    int n, i;
    s3d_navpoint *np;

    if (name) {
        n = level_num_navpoints;
        np = level_navpoints;
        for (i = 0; i < n; np++, i++) {
            if (_stricmp(np->name, name) == 0) {
                *pos = np->pos;
                *orient = np->orient;
                return true;
            }
        }
    }
    return false;
}

// 0x477540  Confidence: exact.
s3d_vis_set *level_vis_record_find(const char *name)
{
    int n = level_num_vis_records;
    s3d_vis_set *v = level_vis_records;
    int i;

    for (i = 0; i < n; v++, i++) {
        if (_stricmp(v->name, name) == 0)
            return v;
    }
    return NULL;
}

// 0x477580  Confidence: exact.
s3d_named_path *level_trigger_record_find(const char *name)
{
    int n = level_s3d.num_named_paths;
    s3d_named_path *p = level_s3d.named_paths;
    int i;

    for (i = 0; i < n; p++, i++) {
        if (_stricmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

// 0x4775c0  Confidence: exact.
int lobj_get_layer(int lobj_index)
{
    return lobjs[lobj_index].get_layer_n(0);
}

// 0x4775e0  Confidence: exact (VC6 byte match) (one dword store of -1; a constructor: it returns this; D803).
lobj::lobj()
{
    *(int *)layers = -1;
}

// 0x4775f0  Confidence: exact. ret 4.
int lobj::get_layer_n(int n)
{
    return layers[n];
}

// 0x477600  Confidence: exact. ret 8. Only found[0] is initialised. Two navigable layers: the
// layer-transition edges decide.
int lobj::pick_layer(const vector3 *pos, int r)
{
    int found[2];
    int n, i, layer;

    if (layers[1] == -1)
        return layers[0];
    found[0] = -1;
    n = 0;
    for (i = 0; i < 2; i++) {
        layer = layers[i];
        if (pf_point_is_navigable(pos, r, layer))
            found[n++] = layer;
    }
    if (n == 1)
        return found[0];
    if (n == 2)
        return pf_layer_from_edges(found[0], found[1], pos);
    return layers[1];
}

// 0x4776a0  Confidence: exact. ret 8.
void lobj::set_layer_n(int n, int layer)
{
    layers[n] = (signed char)layer;
}

// 0x4776b0  Confidence: exact.
bool lobj_index_out_of_range(int index)
{
    if (index >= num_lobjs)
        return true;
    return false;
}

// 0x4776c0  Confidence: exact.
void lobj_get_bbox(int index, vector3 *mins, vector3 *maxs)
{
    *mins = lobjs[index].bbox_min;
    *maxs = lobjs[index].bbox_max;
}

// 0x477720  Confidence: exact (VC6 byte match) (the radius is the mesh's +0x30 float).
void lobj_get_bounding_sphere(int index, vector3 *centre, float *radius)
{
    *centre = lobjs[index].centre;
    *radius = lobjs[index].mesh->_unk30;
}

// 0x477770  Confidence: exact.
int lobj_get_count()
{
    return num_lobjs;
}

// 0x477780  Confidence: exact.
lobj *lobj_from_index(int index)
{
    return &lobjs[index];
}

// 0x4777a0  Confidence: exact.
int lobj_find_by_name(const char *name)
{
    int i;

    for (i = 0; i < num_lobjs; i++) {
        if (_stricmp(name, lobjs[i].name) == 0)
            return i;
    }
    return -1;
}

// 0x4777f0  Confidence: exact.
void lobjs_hide_by_prefix(const char *prefix)
{
    int len = strlen(prefix);
    int i;

    for (i = 0; i < num_lobjs; i++) {
        if (_strnicmp(prefix, lobjs[i].name, len) == 0)
            lobjs[i].flags |= LOBJ_FLAG_HIDDEN;
    }
}

// 0x477850  Confidence: exact.
void lobjs_show_by_prefix(const char *prefix)
{
    int len = strlen(prefix);
    int i;

    for (i = 0; i < num_lobjs; i++) {
        if (_strnicmp(prefix, lobjs[i].name, len) == 0)
            lobjs[i].flags &= ~LOBJ_FLAG_HIDDEN;
    }
}
