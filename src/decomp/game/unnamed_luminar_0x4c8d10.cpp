// unnamed_luminar_0x4c8d10.cpp - Sum.exe v1.40, code 0x4c8d10-0x4ca6f3: the Luminar boss of Iona (species
// 0x51): melee, a grab-and-throw in stage 2, a beam and (stage 3) teleports; and ai_predict_owner_position.
// File name NOT known: a placeholder (src/decomp/README.md). No assert path, no strings.tbl block; cut by its
// static initialiser run (entries 336-337, 0x588544-0x588548) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_luminar_timers (0x4c8d10) to ai_predict_owner_position (0x4ca650-0x4ca6f3).
// level_scripts_common.cpp opens with its initialiser 0x4ca700 (entry 338). ai_predict_owner_position (an
// AI helper the AI combat unit and the lpalaceint script call) has no data; it sits before that
// initialiser, so it ends this unit.
// Data: .data 0x59dfd0-0x59e0e3 (luminar_beam_bm = -1, the command help, the four mesh tag names,
// "GlowLuminarSkin02.vbm", the teleport and beam effect names; "stage" and "$tag-grab-point" are pooled
// with the Giant Salamanka's / tentacle unit's strings); .bss 0x248cdb0-0x248ce3b (luminar_move_anim first,
// ccmd_stage last; 0x248cdb4 is never accessed).
// Sub-modes: 0xf melee, 0x16 close in, 0x32 beam, 0x33 teleport out, 0x34 teleport in, 0x35 the grab move,
// 0x36 the grab approach, 0x37 the release. "Held" (luminar_thrown_handle) is the thrown character in flight,
// luminar_grabbed_handle the carried one (sum.symbols' names).

#include <math.h>
#include <stdlib.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/effects/fx.h"
#include "engine/camera/camera.h"
#include "engine/rendering/levelrender.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define AIS_CLOSE_COMBAT        0x0f  // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE           0x16
#define AIS_LUMINAR_BEAM        0x32  // names ours
#define AIS_LUMINAR_TELEPORT    0x33
#define AIS_LUMINAR_ARRIVE      0x34
#define AIS_LUMINAR_GRAB        0x35
#define AIS_LUMINAR_GRAB_APPROACH 0x36
#define AIS_LUMINAR_RELEASE     0x37

#define CF_LUMINAR_HELD         0x1028    // Character +0x320 bits of the carried character

// ---- .data --------------------------------------------------------------------------------------------
int luminar_beam_bm = -1;             // 0x59dfd0

// ---- .bss (address order) -----------------------------------------------------------------------------
int  luminar_move_anim;               // 0x248cdb0
int  luminar_248cdb4;                 // 0x248cdb4 never accessed
int  luminar_stage;                   // 0x248cdb8
int  luminar_forced_stage;            // 0x248cdbc
vector3 luminar_bite_tag_pos;         // 0x248cdc0 object space
vector3 luminar_pickup_tag_pos;       // 0x248cdcc object space
float luminar_melee_reach;            // 0x248cdd8
float luminar_pickup_reach;           // 0x248cddc
int  luminar_grabbed_handle;           // 0x248cde0 the carried character
int  luminar_thrown_handle;             // 0x248cde4 the thrown character in flight
// 0x4c8d10  Confidence: exact. static_init_luminar_timers: "jmp 0x4c8d20" (initialiser entry 336).
// 0x4c8d20  Confidence: likely. init_luminar_timers: the GameTimer constructor for the release, teleport,
// beam, retarget and attack timers (the last a tail jump).
GameTimer luminar_release_timer;      // 0x248cde8
GameTimer luminar_teleport_timer;     // 0x248cdec
GameTimer luminar_beam_timer;         // 0x248cdf0
vector3 luminar_throw_velocity;       // 0x248cdf4
float luminar_grab_release_health;    // 0x248ce00
float luminar_throw_gravity;          // 0x248ce04
float luminar_grab_cooldown;          // 0x248ce08
bool luminar_beam_hit_done;           // 0x248ce0c
bool luminar_beam_fx_variant;         // 0x248ce0d
vector3 luminar_teleport_dest;        // 0x248ce10
vector3 luminar_teleport_dir;         // 0x248ce1c
GameTimer luminar_retarget_timer;     // 0x248ce28
GameTimer luminar_attack_timer;       // 0x248ce2c

// 0x4c8d60  Confidence: exact. static_init_ccmd_stage: "jmp 0x4c8d70" (entry 337).
// 0x4c8d70  Confidence: exact. init_ccmd_stage (console_command constructor).
console_command ccmd_stage("stage", "Set Luminar's current combat stage", cmd_stage);   // 0x248ce30

// 0x4c8d90  Confidence: exact. 1-3 only, no message (as cmd_gs_stage).
void cmd_stage()
{
    process_command(ARG_INT);
    if ((arg_type & ARG_INT) && command_argument_int > 0 && command_argument_int < 4)
        luminar_forced_stage = command_argument_int;
}

// 0x4c8dc0  Confidence: likely (the zero vector and identity matrix are the object frame of the tag
// queries; the y components are zeroed before the lengths, which are horizontal). The boss becomes
// object type 9; the bite and pickup tags are kept in object space; the melee reach is the mean horizontal
// distance of the two swing tags, the pickup reach the pickup tag's horizontal distance + 0.02.
void ai_state::luminar_boss_load()
{
    vector3 zero;
    matrix identity;
    matrix out;
    vector3 back;
    vector3 swing;
    int tag;
    ca_character *ch;
    float x, z;

    luminar_stage = 1;
    luminar_forced_stage = 0;
    zero.zero();
    identity.make_identity();
    luminar_thrown_handle = -1;
    luminar_grabbed_handle = -1;
    luminar_release_timer.SetTimeout(0);
    luminar_teleport_timer.SetTimeout(0);
    owner->type = 9;
    owner->flags |= CF_COMBAT_READY;
    ch = owner->instance->character;
    tag = ch->find_mesh_tag("$tag-30-bite");
    owner->instance->get_bone_world_transform(tag, &identity, &zero, &out, &luminar_bite_tag_pos);
    tag = ch->find_mesh_tag("$tag-22-pickup");
    owner->instance->get_bone_world_transform(tag, &identity, &zero, &out, &luminar_pickup_tag_pos);
    x = luminar_pickup_tag_pos.x;
    z = luminar_pickup_tag_pos.z;
    luminar_pickup_reach = (float)sqrt(z * z + x * x) + 0.02f;
    tag = ch->find_mesh_tag("$tag-28-backswing-l");
    owner->instance->get_bone_world_transform(tag, &identity, &zero, &out, &back);
    tag = ch->find_mesh_tag("$tag-24-swing-r");
    owner->instance->get_bone_world_transform(tag, &identity, &zero, &out, &swing);
    swing.y = 0.0f;
    back.y = 0.0f;
    luminar_melee_reach = (back.len() + swing.len()) * 0.5f;
    if (luminar_beam_bm < 0)
        luminar_beam_bm = bm_load("GlowLuminarSkin02.vbm", -1, -1);
    owner->combat.set_owner(owner);
    luminar_retarget_timer.SetTimeout(0);
    luminar_attack_timer.SetTimeout(0);
    go_idle();
}

// 0x4c8fd0  Confidence: exact. boss_ai_unload case 0x51: the skin and the level's light scale.
void luminar_free_bitmap()
{
    if (luminar_beam_bm >= 0) {
        bm_release(luminar_beam_bm);
        luminar_beam_bm = -1;
    }
    level_light_scale = 1.0f;
}

// 0x4c9000  Confidence: likely (D15802: the vector operators as the binary's inline budget shows them, its
// copy constructor and (x, y, z) constructor called out of line from ray.start on; the landing in an if block;
// left: ray.start's loads go through EDI, the binary reads them off ESI). The
// carried character follows the grab bone (pickup tag height below it), and its velocity is measured for
// the throw; a thrown one flies (horizontal damping 0.75 per second, gravity 9.8 per second per second)
// until the level ray hits (any flag) or it leaves the navigable area (it is put on the nearest navigable
// point), then lands as the tentacle victims do (without the grab timer).
void luminar_frame_hook(Character *c)
{
    Character *t;
    Character *h;
    vector3 p;
    float damp;
    bool landed;
    int tag;

    t = living_from_handle(luminar_grabbed_handle);
    if (t) {
        tag = c->instance->character->find_mesh_tag("$tag-grab-point");
        c->instance->get_bone_world_transform(tag, &c->orient, &c->pos, &t->orient, &p);
        p -= t->orient.uvec * luminar_pickup_tag_pos.y;
        vector3 d;
        d = p - t->pos;
        luminar_throw_gravity = 0.0f;
        luminar_throw_velocity = d / reported_frametime;
        t->pos = p;
    }
    h = living_from_handle(luminar_thrown_handle);
    if (h == NULL)
        return;
    landed = false;
    lobj_ray ray;
    lobj_hit hit;
    ray.start = h->pos;
    ray.dir = luminar_throw_velocity * reported_frametime;
    ray._unk18 = 0.01f;
    ray.flags = 0;
    ray.require_flags = 0;
    ray.exclude_flags = 0;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits) {
        vector3 step = ray.dir * hit.t;
        h->pos = step + h->pos;
        h->pos.y += 0.5f;
        landed = true;
    } else {
        h->pos.x += ray.dir.x;
        h->pos.y += ray.dir.y;
        h->pos.z += ray.dir.z;
    }
    if (!pf_point_is_navigable(&h->pos, h->info->size, h->layer)) {
        pf_find_nearest_navigable_point(&h->pos, h->info->size, h->layer);
        landed = true;
    }
    damp = 1.0f - reported_frametime * 0.75f;
    luminar_throw_velocity.x *= damp;
    luminar_throw_velocity.z *= damp;
    luminar_throw_velocity.y = luminar_throw_velocity.y - luminar_throw_gravity * reported_frametime;
    luminar_throw_gravity = reported_frametime * 9.8f + luminar_throw_gravity;
    if (landed) {
        h->flags &= ~CF_LUMINAR_HELD;
        h->ai.go_idle();
        vector3 fwd;
        fwd = h->orient.fvec;
        vector3 up;
        up.x = 0.0f;
        up.y = 1.0f;
        up.z = 0.0f;
        fwd.y = 0.0f;
        h->orient.set_from_forward_up(fwd, up);
        h->find_ground();
        luminar_thrown_handle = -1;
        move_info *m = h->find_move(8, 1);
        if (m)
            h->set_flag_2_4(h->play_move((int)m, 1.0f, false, false, false));
        else
            h->clear_flag_2_4();
    }
}

// 0x4c9410  Confidence: exact (VC6 byte match) (no return value: the eax left by the grab branch is not read). The stage
// from HP (thirds), which also dims the level (level_light_scale = HP fraction); the forced stage
// overrides it. Not carrying: stage 2 starts the grab approach (30 s until the forced release), stage 3
// teleports when its timer allows, any stage beams when the beam timer allows; carrying: the release in
// another stage, after 250 HP of damage or after the 30 s. At HP <= 0 in a cutscene: idle and CF_HIDDEN.
void ai_state::luminar_ai_select_action(Character *t)
{
    float frac = owner->hp / (float)owner->get_max_stat(0);

    level_light_scale = frac;
    if (frac < 0.3333f) {
        luminar_stage = 3;
    } else {
        luminar_stage = 2;
        if (!(frac < 0.6667f))
            luminar_stage = 1;
    }
    luminar_grab_cooldown = luminar_grab_cooldown - reported_frametime;
    if (luminar_forced_stage)
        luminar_stage = luminar_forced_stage;
    if (living_from_handle(luminar_grabbed_handle) == NULL) {
        if (living_from_handle(luminar_thrown_handle) == NULL && t && luminar_stage == 2) {
            if (submode != AIS_LUMINAR_GRAB_APPROACH && submode != AIS_LUMINAR_GRAB) {
                luminar_try_start_grab(t);
                luminar_grab_cooldown = 30.0f;
                return;
            }
        } else if (luminar_stage == 3 && luminar_teleport_timer.elapsed() && luminar_teleport_out()) {
            return;
        }
        if (luminar_beam_timer.elapsed() && t && luminar_beam_start(t))
            return;
    } else if (living_from_handle(luminar_thrown_handle) == NULL) {
        if (luminar_stage != 2 || owner->hp <= luminar_grab_release_health || luminar_grab_cooldown < 0.0f) {
            luminar_start_release();
            return;
        }
    }
    if (owner->hp <= 0.0f && (game_flags & GF_CUTSCENE) && !(owner->flags & CF_HIDDEN)) {
        go_idle();
        owner->flags |= CF_HIDDEN;
    }
}

// 0x4c95d0  Confidence: exact. boss_ai_idle_dispatch case 0x51: the host / single player chooses (no
// target).
void ai_state::luminar_ai_idle()
{
    if (!MP_IS_CLIENT())
        luminar_ai_select_action(NULL);
}

// 0x4c95f0  Confidence: exact (VC6 byte match, match22; the edge offset in a float local; was likely) (the sub-mode switch from 0xf, byte table 0x4c977c). Melee and the approach
// tell the frame whether to choose afterwards; the scripted sub-modes do not. The close-in range is the
// melee reach measured from the centre (the edge distance's difference added back).
void ai_state::luminar_ai_frame()
{
    bool choose = true;
    Character *t;

    t = living_from_handle(target);
    if (t && (t->flags & CF_HIDDEN))
        t = NULL;
    switch (submode) {
    case AIS_CLOSE_COMBAT:
        if (luminar_stage != 3)
            luminar_beam_timer.SetTimeout(6000);
        attack_range = owner->info->attack_radius;
        choose = luminar_ai_melee(t);
        break;
    case AIS_GET_CLOSE:
        if (living_from_handle(luminar_grabbed_handle) == NULL && luminar_stage != 3) {
            if (t) {
                float d = distance(&owner->pos, &t->pos);
                float off = d - owner->distance_between_edges(t, false, -1);
                attack_range = luminar_melee_reach - off;
            }
            combat_close_in(t);
        } else if (t && !(owner->distance_between_edges(t, true, -1) > attack_range)) {
            set_mode(mode, AIS_CLOSE_COMBAT);
        }
        break;
    case AIS_LUMINAR_GRAB_APPROACH:
        choose = luminar_ai_grab_approach(t);
        break;
    case AIS_LUMINAR_GRAB:
        luminar_ai_grab(t);
        choose = false;
        break;
    case AIS_LUMINAR_RELEASE:
        luminar_ai_release();
        choose = false;
        break;
    case AIS_LUMINAR_TELEPORT:
        luminar_teleport_in();
        choose = false;
        break;
    case AIS_LUMINAR_ARRIVE:
        luminar_ai_beam_move_end();
        choose = false;
        break;
    case AIS_LUMINAR_BEAM:
        luminar_beam_hit(t);
        choose = false;
        break;
    }
    if (!MP_IS_CLIENT() && choose)
        luminar_ai_select_action(t);
}

// 0x4c97b0  Confidence: exact (VC6 byte match) (the move list is a local array; the count is reloaded for the pick). While
// carrying: move 0x88. Otherwise 0x84 inside the melee reach, 0x85 not more than 0.5 inside it, 0x83 when
// the target stands at the bite tag (horizontal distance squared below 0.15); one of them at random.
int ai_state::luminar_choose_melee_move(Character *t)
{
    int moves[3];
    int count = 0;
    float d;

    if (living_from_handle(luminar_grabbed_handle))
        return 0x88;
    d = distance(&owner->pos, &t->pos) - luminar_melee_reach;
    if (d < 0.0f) {
        moves[0] = 0x84;
        count = 1;
    }
    if (d > -0.5f)
        moves[count++] = 0x85;
    vector3 v = t->pos - owner->pos - luminar_bite_tag_pos;
    if (v.x * v.x + v.z * v.z < 0.15f)
        moves[count++] = 0x83;
    return moves[(int)(frand() * count)];
}

// 0x4c98a0  Confidence: exact (VC6 byte match, match22; the edge distance compared first; was likely). Sub-mode AIS_CLOSE_COMBAT: out of reach (+0.19) back to closing in; once
// facing, the combat update with the chosen move (its result is returned), both attack slots and the
// guards. The current move's progress is read and dropped.
bool ai_state::luminar_ai_melee(Character *t)
{
    float progress;
    bool result;

    if (t == NULL) {
        go_idle();
        return true;
    }
    if (owner->distance_between_edges(t, true, -1) > owner->info->attack_radius + 0.19f) {
        set_mode(AI_STATE_COMBAT, AIS_GET_CLOSE);
        return false;
    }
    if (!combat_face_target(t))
        return true;
    ai_move_flags |= 4;
    result = owner->combat.update(luminar_choose_melee_move(t));
    if (owner->get_current_move())
        owner->move_playing(owner->cur_anim, &progress);
    t->ai.claim_attack_slot(t, owner, 0, -1);
    claim_attack_slot(owner, t, 0, -1);
    t->ai.alert_guards_of_target(owner);
    return result;
}

// 0x4c9990  Confidence: exact. Only party-type targets (with game flag 8: any but species 4 and those with
// Character flag 0x100). The path is dropped, sub-mode AIS_LUMINAR_GRAB_APPROACH; the host sends SWI_BSS.
bool ai_state::luminar_try_start_grab(Character *t)
{
    if (t == NULL)
        return false;
    if (!(game_flags & 0x8)) {
        if (t->type != 7)
            return false;
    } else {
        if (t->info->species == 4)
            return false;
        if (t->flags & 0x100)
            return false;
    }
    ai_path_free(path);
    path = NULL;
    set_mode(AI_STATE_COMBAT, AIS_LUMINAR_GRAB_APPROACH);
    priority = 99999;
    if (MP_IS_HOST())
        mp_send_swi_boss(owner, AI_STATE_COMBAT, AIS_LUMINAR_GRAB_APPROACH);
    return true;
}

// 0x4c9a40  Confidence: exact (VC6 byte match). Sub-mode AIS_LUMINAR_GRAB_APPROACH: chase to the pickup reach, step to the
// side point for the pickup tag, turn, then the grab move (0, 0x86) and sub-mode AIS_LUMINAR_GRAB. True:
// the frame chooses afterwards.
bool ai_state::luminar_ai_grab_approach(Character *t)
{
    int r;

    if (t == NULL) {
        go_idle();
        return true;
    }
    r = chase_target_until_in_range(t, luminar_pickup_reach);
    if (r < 0) {
        go_idle();
        return true;
    }
    if (r == 0)
        return true;
    offset_point_sideways(&t->pos, &luminar_pickup_tag_pos);
    if (!turn_towards(&travel_dest, 0) && owner->get_current_move() == NULL) {
        move_info *m = owner->find_move(0, 0x86);
        luminar_move_anim = owner->play_move((int)m, 1.0f, false, false, false);
        set_mode(AI_STATE_COMBAT, AIS_LUMINAR_GRAB);
        priority = 99999;
        ai_move_flags |= 4;
    }
    return false;
}

// 0x4c9b00  Confidence: exact (VC6 byte match). Sub-mode AIS_LUMINAR_GRAB: at trigger 7 the target takes 96 HP and is
// carried (held pose 0xe, 0x8c; released after 250 more HP of the boss's damage); the boss switches to its
// carrying anims (0xe, 0x8d).
void ai_state::luminar_ai_grab(Character *t)
{
    move_info *m;
    int anim;

    ai_move_flags |= 4;
    if (owner->move_finished(luminar_move_anim)) {
        go_idle();
        return;
    }
    if (!owner->move_trigger_fired(7, 0) || t == NULL)
        return;
    t->add_hp(-96.0f, owner->handle, 1, false);
    m = charinfo_find_move(t->info, 0xe, 0x8c, true);
    if (m == NULL || (anim = m->anim) == -1)
        return;
    t->instance->clear_state_anims();
    t->instance->set_state_weight(anim, 1.0f);
    luminar_grabbed_handle = t->handle;
    luminar_grab_release_health = owner->hp - 250.0f;
    t->set_flag_2_4(-1);
    t->flags |= CF_LUMINAR_HELD;
    m = charinfo_find_move(owner->info, 0xe, 0x8d, true);
    owner->anim_ready = m->anim;
    owner->anim_ready_hurt = m->anim;
    owner->update_locomotion_anim();
}

// 0x4c9c40  Confidence: exact. The release move (0, 0x87) and sub-mode AIS_LUMINAR_RELEASE; the host sends
// the object update and SWI_BSS.
void ai_state::luminar_start_release()
{
    move_info *m = owner->find_move(0, 0x87);
    luminar_move_anim = owner->play_move((int)m, 1.0f, true, false, false);
    set_mode(AI_STATE_COMBAT, AIS_LUMINAR_RELEASE);
    priority = 99999;
    ai_move_flags |= 4;
    if (MP_IS_HOST()) {
        mp_send_obj_update(owner, 0);
        mp_send_swi_boss(owner, AI_STATE_COMBAT, AIS_LUMINAR_RELEASE);
    }
}

// 0x4c9cd0  Confidence: exact (VC6 byte match, match22; logic fix D8101; was likely). Sub-mode AIS_LUMINAR_RELEASE: at trigger 8 the carried character is thrown
// (luminar_frame_hook flies it with the measured velocity), the boss goes back to its anims (move 0xe, 0x93
// for both the ready and the ready-hurt anim: one charinfo_find_move, D8101) and the camera follows a thrown
// cur_character.
void ai_state::luminar_ai_release()
{
    Character *h;
    move_info *m;

    ai_move_flags |= 4;
    if (owner->move_finished(luminar_move_anim)) {
        go_idle();
        return;
    }
    if (!owner->move_trigger_fired(8, 0))
        return;
    luminar_thrown_handle = luminar_grabbed_handle;
    luminar_grabbed_handle = -1;
    luminar_release_timer.SetTimeout(10000);
    m = charinfo_find_move(owner->info, 0xe, 0x93, true);
    owner->anim_ready = m->anim;
    owner->anim_ready_hurt = m->anim;
    owner->update_locomotion_anim();
    h = living_from_handle(luminar_thrown_handle);
    if (h == cur_character)
        game_camera.set_target(h);
}

// 0x4c9da0  Confidence: exact (VC6 byte match, m-1020, D11101: `ok` / `busy` flags instead of the goto pair,
// so the no-point block comes before the found block and the busy return shares its tail; the candidate
// list holds 8; the index is drawn with rand() & 7 until it fits). Five tries: a navigable point 14 from a random party-type entity (with game flag 8 any living
// entity, the boss included) in a random horizontal direction; then the vanish move (8, 0x8a), its effect,
// sub-mode AIS_LUMINAR_TELEPORT, that entity becomes the target and the next teleport waits 20 s
// (0.5 s without a point).
bool ai_state::luminar_teleport_out()
{
    Character *list[8];
    Character *c;
    Character *t;
    vector3 d;
    int count = 0;
    int tries;
    int i;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type == 7 || (game_flags & 0x8)) {
            list[count++] = c;
            if (count >= 8)
                break;
        }
    }
    bool ok = false;
    bool busy = false;
    if (count != 0) {
        if (owner->get_current_move())
            busy = true;
        else {
            tries = 4;
            do {
                do {
                    i = rand() & 7;
                } while (i >= count);
                t = list[i];
                float z = frand() - 0.5f;
                d.x = frand() - 0.5f;
                d.y = 0.0f;
                d.z = z;
                if (d.normalize_length() != 0.0f) {
                    d.x *= 14.0f;
                    d.y *= 14.0f;
                    d.z *= 14.0f;
                    d.x += t->pos.x;
                    d.y += t->pos.y;
                    d.z += t->pos.z;
                    if (pf_point_is_navigable(&d, t->info->size, t->layer)) {
                        ok = true;
                        break;
                    }
                }
            } while (tries-- != 0);
        }
    }
    if (!ok) {
        if (!busy)
            luminar_teleport_timer.SetTimeout(500);
        return false;
    }
    luminar_teleport_dest = d;
    luminar_teleport_dir = t->pos - d;
    move_info *m = owner->find_move(8, 0x8a);
    luminar_move_anim = owner->play_move((int)m, 1.0f, false, false, false);
    fx_attach_to_character("LuminarTeleportOut.vfx", owner->handle, -1, -1, 0, 0);
    set_mode(AI_STATE_COMBAT, AIS_LUMINAR_TELEPORT);
    priority = 99999;
    target = t->handle;
    luminar_teleport_timer.SetTimeout(20000);
    owner->set_6ac(0.0f);
    ai_move_flags |= 4;
    if (MP_IS_HOST())
        mp_send_swi_boss(owner, AI_STATE_COMBAT, AIS_LUMINAR_TELEPORT);
    return true;
}

// 0x4ca000  Confidence: exact (VC6 byte match) (as murod_ai_leap, with the move trigger 6 polled and the effects). Sub-mode
// AIS_LUMINAR_TELEPORT: when the vanish move ends, the boss appears at the destination facing the target
// with the arrival move (8, 0x8b) and "LuminarTeleportIn.vfx" (sub-mode AIS_LUMINAR_ARRIVE); until then the
// tint and alpha fade from frame 73.
void ai_state::luminar_teleport_in()
{
    float t;
    float k;
    int alpha;

    ai_move_flags |= 4;
    if (owner->move_finished(luminar_move_anim)) {
        owner->pos = luminar_teleport_dest;
        owner->flags &= ~CF_ON_GROUND;
        owner->find_ground();
        owner->orient.set_from_forward(luminar_teleport_dir);
        owner->obj_flags |= 1;
        owner->instance->_unkb4 = 0xff;
        move_info *m = owner->find_move(8, 0x8b);
        luminar_move_anim = owner->play_move((int)m, 1.0f, false, false, false);
        fx_attach_to_character("LuminarTeleportIn.vfx", owner->handle, -1, -1, 0, 0);
        set_mode(AI_STATE_COMBAT, AIS_LUMINAR_ARRIVE);
        priority = 99999;
        return;
    }
    owner->move_trigger_fired(6, 0);
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

// 0x4ca1d0  Confidence: exact. Sub-mode AIS_LUMINAR_ARRIVE: after the arrival move, stage 3 beams at once
// at a visible target.
void ai_state::luminar_ai_beam_move_end()
{
    Character *t;

    ai_move_flags |= 4;
    if (!owner->move_finished(luminar_move_anim))
        return;
    if (luminar_stage == 3) {
        t = living_from_handle(target);
        if (t && !(t->flags & CF_HIDDEN) && luminar_beam_start(t))
            return;
    }
    go_idle();
}

// 0x4ca230  Confidence: exact (VC6 byte match). Range 5-20 (squared 25-400; a multiplayer client skips the test). The beam
// move (0, 0x89; a client plays it with keep), the source effect (the lightning bolt when the heights differ
// by more than 2, else "LuminarBeam-source.vfx"), sub-mode AIS_LUMINAR_BEAM; the host sends SWI_BSS.
bool ai_state::luminar_beam_start(Character *t)
{
    float d2 = distance_squared(&owner->pos, &t->pos);
    move_info *m;

    if (!MP_IS_CLIENT()) {
        if (d2 < 25.0f || d2 > 400.0f)
            return false;
    }
    m = owner->find_move(0, 0x89);
    if (MP_IS_CLIENT())
        luminar_move_anim = owner->play_move((int)m, 1.0f, true, false, false);
    else
        luminar_move_anim = owner->play_move((int)m, 1.0f, false, false, false);
    if ((float)fabs(owner->pos.y - t->pos.y) > 2.0f) {
        fx_attach_to_character("spell-lbolt-source.vfx", owner->handle, -1, -1, 0, 0);
        luminar_beam_fx_variant = true;
    } else {
        fx_attach_to_character("LuminarBeam-source.vfx", owner->handle, -1, -1, 0, 0);
        luminar_beam_fx_variant = false;
    }
    set_mode(AI_STATE_COMBAT, AIS_LUMINAR_BEAM);
    priority = 99999;
    luminar_beam_timer.reset();
    luminar_beam_hit_done = false;
    owner->set_6ac(0.0f);
    ai_move_flags |= 4;
    if (MP_IS_HOST())
        mp_send_swi_boss(owner, AI_STATE_COMBAT, AIS_LUMINAR_BEAM);
    return true;
}

// 0x4ca3a0  Confidence: exact (VC6 byte match, match22; side vector, else-block scope; was likely). Sub-mode AIS_LUMINAR_BEAM: until it hits, the boss turns to a point 1.05 to
// the side of the target; trigger 11 calls the instance's empty skin hook with the glow skin; trigger 7 hits
// once for 125 HP (a surviving target flinches and gets the target effect: the bolt, or the beam effect
// facing back along the boss's facing). The trigger-7 branch does not test the target for NULL (NOTES, game
// bugs). At the end, stage 3 rearms the beam for 20 s.
void ai_state::luminar_beam_hit(Character *t)
{
    vector3 d;

    ai_move_flags |= 4;
    if (t && !luminar_beam_hit_done) {
        d = t->pos - owner->pos;
        d.normalize();
        travel_dest = vector3(-d.z, 0.0f, d.x) * 1.05f + t->pos;
        turn_towards(&travel_dest, 0);
    }
    if (owner->move_finished(luminar_move_anim)) {
        luminar_beam_hit_done = false;
        if (luminar_stage == 3) {
            luminar_beam_timer.SetTimeout(20000);
            go_idle();
        }
        go_idle();
        return;
    }
    if (owner->move_trigger_fired(11, 0))
        owner->instance->ca_noop_2arg(luminar_beam_bm, 0x3fe66666);   // the float 1.8f (the stub takes int)
    if (!owner->move_trigger_fired(7, 0))
        return;
    luminar_beam_hit_done = true;
    if (!t->add_hp(-125.0f, owner->handle, 1, false))
        return;
    t->combat.play_flinch(2);
    if (luminar_beam_fx_variant) {
        fx_attach_to_character("spell-lbolt-target.vfx", t->handle, -1, -1, 0, 0);
    } else {
        matrix m;
        vector3 back(-owner->orient.fvec.x, -owner->orient.fvec.y, -owner->orient.fvec.z);
        m.set_from_forward_up(back, owner->orient.uvec);
        fx_add_effect("LuminarBeam-target.vfx", &t->pos, &m, 1);
    }
}

// 0x4ca5a0  Confidence: exact. boss_hook_dispatch_51_c1 case 0x51: hold while a move plays or the attack
// timer runs.
bool luminar_hold_attack(Character *boss, Character *target)
{
    if (boss->get_current_move())
        return true;
    return luminar_attack_timer.elapsed() ? false : true;
}

// 0x4ca5c0  Confidence: exact. ai_retaliate_on_hit's call: at most one switch every 5 s.
bool luminar_can_switch_target()
{
    if (luminar_retarget_timer.elapsed() == true) {
        luminar_retarget_timer.SetTimeout(5000);
        return true;
    }
    return false;
}

// 0x4ca5f0  Confidence: exact. The next attack waits at least the rest of the current move + 100 ms.
void luminar_delay_next_attack(Character *c)
{
    float t;
    int ms = 0;
    int left;

    if (c->get_current_move() && c->move_playing(c->cur_anim, &t))
        ms = (int)(t * 1000.0f);
    ms += 100;
    left = luminar_attack_timer.GetRemainingTime();
    if (left <= ms)
        left = ms;
    luminar_attack_timer.SetTimeout(left);
}

// 0x4ca650  Confidence: exact (VC6 byte match) (thiscall, ret 8). The owner's position after the given seconds at its
// forward speed along its facing, moved onto the navigation mesh when it leaves it.
void ai_state::predict_owner_position(vector3 *out, float seconds)
{
    float s = owner->get_698() * seconds;

    *out = owner->orient.fvec * s + owner->pos;
    if (!pf_point_is_navigable(out, owner->info->size, owner->layer))
        pf_find_nearest_navigable_point(out, owner->info->size, owner->layer);
}
