// gametarget.cpp - Sum.exe v1.40, code 0x43a330-0x43cd6d (the world mouse: what is under the cursor, the
// hover tints, the click on characters / items / containers / doors, the click move, the held-click
// camera turn, chain attack selection).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 623-638). It follows state_help_leave (0x43a300,
// gameplay.cpp) and ends with hud_set_timed_highlight; summoner.cpp starts at 0x43cd70. Inside the range
// are two COMDATs of inline members: vector3(x, y, z) 0x43b580 and plane::set_normal_point 0x43b5a0
// (vsdk/math/vector.h, vsdk/math/ix.h), and the atexit nop of world_mouse_frame's static (0x43cd40).
// Data: .rdata 0x580e00-0x580e37 (the unit's float literals); .data 0x5943f0-0x594437 (the pick filters,
// the tints, then the literals "gold" 0x594430 and "!" 0x594438); .bss 0x60ab60-0x60abab.
// Pooled float literals from earlier units: 0.5 (0x57e6b0), 100 (0x57e6b4), 1 (0x57e990), 10 (0x57eb7c),
// 0 (0x57eb84), 0.2 (0x57ef10), 15 (0x57ef14), 5 (0x57ef18), 3 (0x57ef1c), 1.5 (0x580210), 0.05 (0x58021c).
//
// The input record is player_input {flags, mouse x, mouse y} (player.h). Flag bits as this unit reads
// them: 1 right click (cancel), 4 left click, 8 the move goal flag (world_click_move), 0x10 left button
// held, 0x40000000 debug teleport click, 0x80000000 debug pick of the character under the mouse.

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/ix.h"
#include "vsdk/gr/gr.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/camera/camera.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/spells/spells.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/conversation.h"
#include "levelscripts/savegame.h"
#include "summoner/summoner.h"
#include "summoner/gametarget.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_buysell.h"
#include "summoner/interface/new_inventory.h"
#include "summoner/interface/new_skill.h"
#include "summoner/interface/new_spell.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

#define INPUT_RCLICK     0x1
#define INPUT_CLICK      0x4
#define INPUT_MOVE_FLAG  0x8
#define INPUT_HELD       0x10
#define INPUT_TELEPORT   0x40000000
#define INPUT_DEBUG_PICK 0x80000000

#define DIRECT_ORDER     999999      // ai goal priority of the player's orders (0xf423f)
#define SOUND_CLICK_OK   0x23
#define SOUND_CLICK_BAD  0x24

// ---- .data 0x5943f0-0x594437 ------------------------------------------------------------------------
const char world_pick_default_filter[8] = { 1, 1, 1, 1, 1 };  // 0x5943f0
const char world_pick_skill26_filter[5] = { 0, 1, 1 };        // 0x5943f8
bool  world_mouse_auto_turn = true;                           // 0x5943fd
float world_tint_enemy[3] = { 1.0f, 0.5f, 0.5f };            // 0x594400
float world_tint_party[3] = { 0.75f, 1.0f, 0.75f };          // 0x59440c
float world_hover_tint[3] = { 0.75f, 1.0f, 1.0f };           // 0x594418
float world_tint_gold[3] = { 0.98f, 1.0f, 0.51f };           // 0x594424

// ---- .bss 0x60ab60-0x60abab ------------------------------------------------------------------------
// The two GameTimer constructions are compiler-generated initialiser pairs (static_initializers entries
// 130 and 131). Initialisers run in definition order, so chain_attack_timer is defined first although its
// address is the higher one (as in multitrade.cpp; the definition order against the addresses is unsure).
// 0x43a330  Confidence: exact. init_gametimer_60ab70_thunk: jmp 0x43a340.
// 0x43a340  Confidence: exact. init_gametimer_60ab70: mov ecx, 0x60ab70 / jmp GameTimer__GameTimer.
// 0x43a350  Confidence: exact. init_gametimer_60ab60_thunk: jmp 0x43a360.
// 0x43a360  Confidence: exact. init_gametimer_60ab60: mov ecx, 0x60ab60 / jmp GameTimer__GameTimer.
GameTimer chain_attack_timer;                                 // 0x60ab70
GameTimer gold_highlight_timer;                               // 0x60ab60
// 0x60ab64: world_mouse_frame's static (pick_index).
bool  world_pick_portrait_clicked;                            // 0x60ab68
Character *world_hover_portrait_character;                    // 0x60ab6c
// 0x60ab78: world_mouse_frame's static hit_pos (sum.symbols world_mouse_hit_pos).
int   world_interact_kind;                                    // 0x60ab88
void *world_hover_object;                                     // 0x60ab8c
void *world_interact_object;                                  // 0x60ab90
// 0x60ab94: the guard of hit_pos; 0x60ab98: world_mouse_frame's static hit_face.
int   gold_highlight_handle;                                  // 0x60ab9c
bool  world_click_handled;                                    // 0x60aba0
const char *world_pick_filter;                                // 0x60aba4
int   world_pick_lobj_counter;                                // 0x60aba8

// 0x43a370  Confidence: exact.
void world_pick_set_skill_filter(int skill)
{
    world_pick_filter = world_pick_default_filter;
    if (skill == 0x26)
        world_pick_filter = world_pick_skill26_filter;
}

// 0x43a390  Confidence: exact.
void world_pick_clear_filter()
{
    world_pick_filter = NULL;
}

// 0x43a3a0  Confidence: exact (VC6 byte match) (is_leader compared with true: cmp al, 1). Every party member that is
// the leader, or not on hold and without a target, attacks (in a network game only cur_character).
void party_order_attack(Character *target)
{
    if (cur_character->is_enemy(target)) {
        for (Player *p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (!c)
                continue;
            if ((game_flags & GF_MULTIPLAYER) && c != cur_character)
                continue;
            if (p->is_leader() == true || (!(p->flags & PF_HOLD) && c->ai.target == -1))
                c->ai.add_attack(target, DIRECT_ORDER);
        }
        combat_mode_set(1);
        game_play_sound(SOUND_CLICK_OK, 1.0f, 0, 0, 0);
        return;
    }
    game_play_sound(SOUND_CLICK_BAD, 1.0f, 0, 0, 0);
}

// 0x43a460  Confidence: likely (the order of the length sum's terms; the name test is written on the
// array member, the code tests its address too). Not while a conversation runs or the NPC is dead; on
// level 0x1d not while a type 5 / 9 entity in AI mode 3 targets a party member; then within 5 m between
// the body edges and 0.5 m of height, and a speaker of the conversation file (single player) or a
// shopkeeper (network game). D8901 (LOGIC FIX, match30): the shopkeeper lookup takes npc->name itself
// (0x43a5d2 `push ecx`, ECX = npc + 0x2f4), not the info-name fallback the speaker test uses.
int character_can_talk_to(Character *npc)
{
    if (!conversation_active.is_idle())
        return 0;
    if (npc->flags & CF_DEAD)
        return 0;

    if (cur_level_index == 0x1d) {
        for (Character *c = living_list.live_next; c != &living_list; c = c->live_next) {
            if ((c->type == 5 || c->type == 9) && c->ai.is_mode(3, -1)) {
                for (Player *p = party_list.next; p != &party_list; p = p->next)
                    if (p->handle == c->ai.target)
                        return 0;
            }
        }
    }

    vector3 a = cur_character->pos + cur_character->centre_offset;
    vector3 b = npc->centre_offset + npc->pos;
    vector3 d = b - a;
    if (fabsf(d.y) <= 0.5f) {
    float dist = (float)sqrt(d.x * d.x + d.z * d.z);
    if (!(dist - npc->size_radius() - cur_character->size_radius() < 5.0f))
        return 0;

    char *name = npc->name;
    if (!name || !*name)
        name = (char *)npc->info->name;
    if (game_flags & GF_SINGLE_PLAYER) {
        if (conversation_db.has_speaker(name))
            return 1;
    } else {
        if (level_shopkeeper_find(npc->name))
            return 1;
    }
    }
    return 0;
}

// 0x43a600  Confidence: exact (VC6 byte match) (the index typo). While the 5 s timer of hud_set_timed_highlight runs,
// the dropped object is tinted gold. The blue argument reads world_tint_gold[3], one past the array:
// the four bytes of "gold" (0x594430) as a float, about 1.7e22 (game bug; NOTES). 0x59442c (the 0.51)
// is read nowhere.
void world_tint_gold_object()
{
    if (gold_highlight_timer.valid() && !gold_highlight_timer.elapsed()) {
        obj *o = obj_from_handle(gold_highlight_handle);
        if (o)
            world_hover_tint_object(o, world_tint_gold[0], world_tint_gold[1], *(world_tint_gold + 3));
    }
}

// 0x43a650  Confidence: exact. The colour pulses between 0.5 and 1.0 times the tint.
void world_hover_tint_object(obj *o, float r, float g, float b)
{
    float pb, pg, pr;

    pr = 1.0f;
    pg = 1.0f;
    pb = 1.0f;
    ui_apply_pulse_rgb(&pr, &pg, &pb, 0);
    hover_tint_set_object(o, (pr + 1.0f) * r * 0.5f, (pg + 1.0f) * g * 0.5f, (pb + 1.0f) * b * 0.5f);
}

// 0x43a6e0  Confidence: exact (VC6 byte match, match30; was likely; D8902: the screen argument through a block-scope copy, D7004). Earlier note: (the request record's third field is not written). A client asks the
// host (service request 3 with the container's net handle) and returns; the host tells the others,
// then the pickup screen (4) opens on the container.
void object_open_pickup(obj *o)
{
    ai_service_req req;

    if (MP_IS_CLIENT()) {
        req.type = 3;
        req._unk04 = o->net_handle;
        ai_service_request(&req);
        return;
    }
    if (MP_IS_HOST())
        mp_send_container_open(mp_local_player->id, o->handle);
    {
        obj *open = o;
        interface_set_screen(4, &open);
    }
}

// 0x43a770  Confidence: likely (the loop and the hit test as one condition). The level object the mouse
// ray hits first, as far as the near fog distance, among the drawn opaque ones; *face is the face index
// and *hit the point.
int world_pick_lobj(vector3 *origin, vector3 *dir, vector3 *hit, int *face)
{
    lobj_ray ray;
    lobj_hit h;
    int iter = 0;
    bool flag = false;
    float best = 10000.0f;
    int index = -1;

    do {
        ray.start = *origin;
        ray.dir = *dir * fog_get_far_current();
        ray._unk18 = 0;
        ray.flags = 0;
        ray.require_flags = 0;
        ray.exclude_flags = 0;
        lobj_raycast_draw_lists(&ray, &h, &iter, &flag, false);
        if (h.num_hits && h.t < best && !(lobjs[h.face].flags & 0x400)
            && (levelrender_lobj_states[h.face].flags & 0xff0) == 0xff0) {
            best = h.t;
            index = h.face;
            *face = (s3d_face *)h._unk28 - lobj_from_index(h.face)->mesh->faces;
            *hit = ray.dir * h.t + *origin;
        }
    } while (h.num_hits);
    return index;
}

// 0x43a940  Confidence: exact (level_load).
void world_pick_reset()
{
    world_pick_lobj_counter = 3;
    world_interact_kind = 0;
    world_hover_portrait_character = NULL;
    world_pick_portrait_clicked = false;
}

// An empty function world_mouse_frame calls with (level object, face) (a debug hook; name ours). It has no
// address of its own: identical-code folding merged it into the bare ret at 0x40acc0
// (empty_frame_hook_40acc0, new_gamelist.cpp). Confidence: unsure (D1507).
void world_pick_debug_hook(lobj *l, int face)
{
}

// 0x43cd40 (no sum.symbols name)  Confidence: exact. The atexit handler of world_mouse_frame's static
// hit_pos: a bare ret (VC6 registers one for a function-local static of class type, as for
// levelrender_wave_lobj's GameTimer).

// 0x43a960  Confidence: likely (big; the local layout, the tests of the character distance and the
// statics' kinds). Every frame of state_game_frame: picks the level object (every third frame) and the
// object under the mouse, tints it, and handles the click (world_mouse_hover_target's result).
void world_mouse_frame(unsigned int *input, vector3 *eye)
{
    static vector3 hit_pos;              // 0x60ab78 world_mouse_hit_pos (guard 0x60ab94 bit 0)
    static int pick_index;               // 0x60ab64 the last world_pick_lobj result
    static int hit_face;                 // 0x60ab98 world_pick_hit_lobj (the face index)
    int clip_x, clip_y, clip_w, clip_h;
    ix_ray ray;
    lobj *l;
    obj *picked;
    Character *target;
    lobj *door;
    Player *player;
    bool valid;
    int kind;
    ai_service_req req;
    char msg[128];

    if (cur_character->ai.is_traveling_sub1() && !(input[0] & INPUT_HELD))
        cur_character->ai.go_idle();
    if (cur_character->flags & (CF_SUMMONLIKE | CF_HIDDEN))
        return;
    if (world_interact_kind) {
        object_use_container();
        return;
    }

    world_hover_object = NULL;
    world_interact_object = NULL;
    world_tint_gold_object();
    chain_attack_select(input);
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    gr_screen_to_world_ray(&ray, (float)(int)(input[1] - clip_x), (float)(int)(input[2] - clip_y));
    vector3 dir = ray.dir;

    if ((input[0] & INPUT_HELD) && world_click_handled && !(cur_character->ai.flags & 2)) {
        world_pick_under_mouse(eye, &dir, &hit_pos, input);
        return;
    }
    if (!world_hover_portrait_character && interface_mouse_over_panel())
        return;
    if (input[0] & INPUT_RCLICK) {
        if (hud_target_mode != -1) {
            hud_target_mode = -1;
            return;
        }
        if (!cur_character->ai.is_mode(3, -1))
            sound_play_2d(SOUND_CLICK_BAD, 1.0f);
        return;
    }

    if (++world_pick_lobj_counter >= 3) {
        world_pick_lobj_counter = 0;
        pick_index = world_pick_lobj(eye, &dir, &hit_pos, &hit_face);
    }
    if (pick_index == -1) {
        world_click_handled = true;
        return;
    }

    l = lobj_from_index(pick_index);
    picked = world_hover_portrait_character;
    target = NULL;
    door = NULL;
    player = NULL;
    if (!picked)
        picked = pick_test_character(&hit_pos);
    if (picked) {
        if (picked->is_living() || picked->is_dead_type()) {
            target = (Character *)picked;
            if (picked->type == 7 && target->hp > 0.0f) {
                player = player_from_handle(picked->handle);
            } else if (target->ai.is_mode(6, -1) && (target->flags & CF_SUMMONLIKE)) {
                picked = NULL;
                target = NULL;
            } else if (target->is_mount()) {
                picked = NULL;
                target = NULL;
            } else {
                if (!(~game_camera.flags & 1)
                    && ((cur_character->pos - target->pos).len() >= 15.0f || fabs(cur_character->pos.y - target->pos.y) >= 3.0f)) {
                    picked = NULL;
                    target = NULL;
                }
            }
        }
    } else if (l && (game_flags & GF_SINGLE_PLAYER)) {
        if (l->flags & 0x100) {
            if (door_in_use_range(cur_character, l)) {
                door = l;
                world_hover_tint_character(l, world_hover_tint[0], world_hover_tint[1], world_hover_tint[2]);
            }
        } else {
            for (int i = 0; i < lc_entry_count; i++) {
                lobj *o = lc_entries[i].object;
                if (o->flags & 0x80)
                    continue;
                if ((lc_entries[i].flags & 1) && lc_entries[i].gold < 1)
                    continue;
                vector3 d = o->pos - cur_character->pos;
                if (d.len_sq() <= 25.0f && o == l) {
                    door = l;
                    world_hover_tint_character(l, world_hover_tint[0], world_hover_tint[1],
                                               world_hover_tint[2]);
                    break;
                }
            }
        }
    }
    world_pick_debug_hook(l, hit_face);

    if (cur_level_index != 4) {
        if (!input[0] && !picked && hud_target_mode == -1)
            return;
        if (input[0] & INPUT_TELEPORT) {
            if (l->flags & 0x40)
                party_teleport_to(&hit_pos, l->pick_layer(&hit_pos, 1));
            return;
        }
        if (input[0] & INPUT_DEBUG_PICK) {
            if (target && target->hp > 0.0f) {
                world_mouse_target_handle = target->handle;
                ui_input_lock();
            }
            return;
        }

        kind = world_mouse_hover_target(input, l, door, hit_face, &hit_pos, picked, target, &valid);
        if (kind == WORLD_CLICK_AT_TARGET || kind == WORLD_CLICK_AT_POINT) {
            if ((world_hover_portrait_character && world_pick_portrait_clicked) || (input[0] & INPUT_CLICK)) {
                if (valid) {
                    switch (kind) {
                    case WORLD_CLICK_AT_POINT:
                        if (ui_using_list_item)
                            world_use_item_at_point(ui_using_list_item_entry, &hit_pos,
                                                    l->pick_layer(&hit_pos, 1));
                        else if (!ui_using_equipped_item)
                            character_cast_at_point(ui_pending_use_skill, &hit_pos,
                                                    l->pick_layer(&hit_pos, 1), DIRECT_ORDER, NULL);
                        break;
                    case WORLD_CLICK_AT_TARGET:
                        if (ui_using_list_item)
                            world_use_item_on_object(ui_using_list_item_entry, target);
                        else if (ui_using_equipped_item)
                            world_use_equipped_item_on_object(ui_using_equipped_slot, target);
                        else
                            character_use_skill_on(ui_pending_use_skill, target, DIRECT_ORDER, NULL);
                        break;
                    }
                } else {
                    game_play_sound(SOUND_CLICK_BAD, 1.0f, 0, 0, 0);
                }
            }
        }

        if (input[0] & INPUT_CLICK) {
            world_click_handled = false;
            input[0] &= ~INPUT_CLICK;
            switch (kind) {
            case WORLD_CLICK_NONE:
                input[0] |= INPUT_CLICK;
                break;
            case WORLD_CLICK_SELECT:
                party_select_member(player);
                break;
            case WORLD_CLICK_TALK:
                player_talk_to(target);
                break;
            case WORLD_CLICK_ATTACK:
                party_order_attack(target);
                break;
            case WORLD_CLICK_GOLD:
                if (MP_IS_CLIENT()) {
                    req.type = 2;
                    req._unk04 = picked->net_handle;
                    ai_service_request(&req);
                    break;
                }
                if (MP_IS_HOST())
                    mp_broadcast_container_gold(mp_local_player->id, picked->handle);
                sprintf(msg, XSTR("Picked up %d Gold", 223), ((item_obj *)picked)->count);
                hud_show_message(msg);
                item_obj_pickup((item_obj *)picked, 1);
                break;
            case WORLD_CLICK_CONTAINER:
                if (picked->obj_flags & 4)
                    break;
                if ((game_flags & GF_MULTIPLAYER) && ((container *)picked)->user != -1) {
                    hud_show_message(XSTR("Container is currently in use", 638));
                    break;
                }
                if (((container *)picked)->lock > 0.0f) {
                    hud_show_message(XSTR("Locked", 626));
                    feedback_play(cur_character->info->species, 3, &cur_character->pos);
                    sound_play_2d(5, 1.0f);
                    break;
                }
                object_open_pickup(picked);
                break;
            case WORLD_CLICK_DOOR:
                object_try_unlock_or_open(l);
                break;
            case WORLD_CLICK_EXAMINE:
                container_examine(l);
                break;
            case WORLD_CLICK_PICK_LOCK:
                character_use_skill(SKILL_PICK_LOCK, NULL, NULL);
                break;
            case WORLD_CLICK_TRADE:
                interface_set_screen(0, NULL);
                mp_send_it_rqst(player);
                break;
            case WORLD_CLICK_SHOP:
                shop_set_shopkeeper_from_char(target);
                interface_set_screen(7, NULL);
                break;
            }
        }
    }
    world_click_move(input, &hit_pos, l);
}

// 0x43b1d0  Confidence: exact. world_hover_tint_object for a level object (door, container).
void world_hover_tint_character(lobj *l, float r, float g, float b)
{
    float pb, pg, pr;

    pr = 1.0f;
    pg = 1.0f;
    pb = 1.0f;
    ui_apply_pulse_rgb(&pr, &pg, &pb, 0);
    hover_tint_set_character(l, (pr + 1.0f) * r * 0.5f, (pg + 1.0f) * g * 0.5f, (pb + 1.0f) * b * 0.5f);
}

// 0x43b260  Confidence: likely (the inline vector members: the first ones expanded, the later ones
// called as the COMDATs 0x410cd0 / 0x43b580 / 0x413190; the dead copies of operator='s result). The
// left button held: the mouse ray against the horizontal plane through cur_character (else against the
// plane facing the ray, 10 m out) gives the direction to walk; the speed grows with the cursor's distance
// from the screen centre (with the high camera from the point's distance). With world_mouse_auto_turn, a
// press held over 4 ms also turns the camera that way.
void world_pick_under_mouse(vector3 *eye, vector3 *dir, vector3 *out, unsigned int *input)
{
    float dist = 0.0f;
    float speed;
    int clip_x, clip_y, clip_w, clip_h;
    vector3 up;
    ix_ray ray;
    plane ground;

    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;
    ground.set_normal_point(&up, &cur_character->pos);
    ray.origin = *eye;
    ray.dir = *dir;
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    if (!ix_ray_plane(&ray, &ground, out)) {
        vector3 ahead = *dir * 10.0f;
        vector3 p = ahead + cur_character->pos;
        vector3 n = -*dir;
        plane facing;
        facing.set_normal_point(&n, &p);
        ground = facing;
        if (!ix_ray_plane(&ray, &ground, out))
            return;
    }

    *out -= cur_character->pos;
    if (game_camera.elevation < 4.0f)
        dist = out->len();
    out->normalize();

    float mx = ((float)(int)input[1] / clip_w * 2.0f - 1.0f) * 1.1f;
    float my = ((float)(int)input[2] / clip_h * 2.0f - 1.0f) * 1.1f;
    speed = ((float)sqrt(my * my + mx * mx) - -0.05f) * 0.952381f;
    if (game_camera.elevation < 4.0f) {
        speed = dist * 0.25f;
        if (speed >= 1.0f)
            speed = 1.0f;
    }

    float held = (float)mouse_time_since_press(0);
    cur_character->ai_travel_offset(*out * speed);
    if (world_mouse_auto_turn && held > 4.0f)
        game_camera.wanted_dir = *out;
}

// 0x43b580 vector3::vector3(float, float, float) (vsdk/math/vector.h): the COMDAT of the inline
// constructor, emitted after its first out-of-line use (world_pick_under_mouse). Confidence: exact
// (compiler-generated from the header).

// 0x43b5a0 plane::set_normal_point (vsdk/math/ix.h): the COMDAT of the inline member, emitted after
// world_pick_under_mouse; its -1.0f is this unit's .rdata 0x580e1c. Confidence: exact (compiler-generated
// from the header).

// 0x43b5e0  Confidence: likely (two jump tables: 0x43bb60 by hud_target_mode + 1, 0x43bb70 by object type
// - 3). Tints what is under the mouse and says what a click does (WORLD_CLICK_*); for a targeted skill or
// item it also places the target decal and sets *valid.
//   hud_target_mode -1: the object under the mouse by type (7 party / other player, 4 NPC, 5 and 9
//   monsters, 3 items, 11 containers), else the level object (door 8 / container 9);
//   0: a character for ui_pending_use_skill; 1: a ground point; 2: lock picking (doors, containers).
int world_mouse_hover_target(unsigned int *input, lobj *l, lobj *door, int face, vector3 *hit,
                             obj *picked, Character *target, bool *valid)
{
    if (game_flags & 0x100) {
        hud_target_mode = -1;
        return WORLD_CLICK_NONE;
    }

    switch (hud_target_mode) {
    case -1:
        if (!picked) {
            if (!door)
                return WORLD_CLICK_NONE;
            world_hover_tint_object(NULL, world_hover_tint[0], world_hover_tint[1], world_hover_tint[2]);
            return ((~door->flags & 0x100) | (WORLD_CLICK_DOOR << 8)) >> 8;
        }
        world_hover_object = picked;
        switch (picked->type) {
        case 7: {
            if (!target || target == cur_character || (target->flags & (CF_SUMMONLIKE | CF_HIDDEN | CF_DEAD)))
                return WORLD_CLICK_NONE;
            if (game_flags & GF_MULTIPLAYER) {
                Player *p = player_from_handle(target->handle);
                if (!p || p->mp_owner != mp_local_player) {
                    vector3 d = cur_character->pos - target->pos;
                    if (target->flags & 0x100)
                        return WORLD_CLICK_NONE;
                    if (!(d.len_sq() <= 25.0f))
                        return WORLD_CLICK_NONE;
                    world_hover_tint_object(picked, world_tint_party[0], world_tint_party[1], world_tint_party[2]);
                    return WORLD_CLICK_TRADE;
                }
            }
            world_hover_tint_object(picked, world_tint_party[0], world_tint_party[1], world_tint_party[2]);
            return WORLD_CLICK_SELECT;
        }
        case 4: {
            target->get_name();             // result unused
            if (!character_can_talk_to(target))
                return WORLD_CLICK_NONE;
            world_hover_tint_object(picked, world_hover_tint[0], world_hover_tint[1], world_hover_tint[2]);
            return (game_flags & GF_SINGLE_PLAYER) ? WORLD_CLICK_TALK : WORLD_CLICK_SHOP;
        }
        case 5:
        case 9: {
            int team = target->get_team();
            if (team != 2 && team != 3)
                return WORLD_CLICK_NONE;
            world_hover_tint_object(picked, world_tint_enemy[0], world_tint_enemy[1], world_tint_enemy[2]);
            return WORLD_CLICK_ATTACK;
        }
        case 3:
        case 11: {
            vector3 d = cur_character->pos - picked->pos;
            if (d.len_sq() <= 9.0f) {
                world_hover_tint_object(picked, world_hover_tint[0], world_hover_tint[1], world_hover_tint[2]);
                return WORLD_CLICK_GOLD + (picked->type != 3);
            }
            world_hover_tint_object(picked, world_tint_enemy[0], world_tint_enemy[1], world_tint_enemy[2]);
            return WORLD_CLICK_NONE;
        }
        }
        break;

    case 0: {
        if (!target)
            return WORLD_CLICK_NONE;
        world_hover_object = target;
        bool ok;
        if (target->has_status(0x10))
            ok = spells[ui_pending_use_skill].damage_type != -1;
        else if (spells[ui_pending_use_skill].flags & SPELL_OFFENSIVE)
            ok = cur_character->is_enemy(target);
        else
            ok = picked->type == 7;
        if (target->hp > 0.0f) {
            if (ui_pending_use_skill == 5 || ui_pending_use_skill == 3)
                ok = false;
        } else {
            if (ui_pending_use_skill != 5 && ui_pending_use_skill != 3)
                ok = false;
        }
        if (ui_pending_use_skill == 0xb && target->has_status(8))
            ok = false;
        if (ui_pending_use_skill == 0x45 && !cur_character->ai.can_backstab_target(target))
            ok = false;
        if ((ui_pending_use_skill == -1
             || distance(&target->pos, &cur_character->pos) <= spells[ui_pending_use_skill].distance) && ok) {
            world_hover_tint_object(picked, world_hover_tint[0], world_hover_tint[1], world_hover_tint[2]);
            *valid = world_pick_ground_point(cursor_spot_target_bm, cursor_spot_invalid_bm, l, face,
                                             &picked->pos, false);
            return WORLD_CLICK_AT_TARGET;
        }
        world_hover_tint_object(picked, world_tint_enemy[0], world_tint_enemy[1], world_tint_enemy[2]);
        *valid = false;
        world_pick_ground_point(cursor_spot_invalid_bm, cursor_spot_invalid_bm, l, face, &picked->pos, false);
        return WORLD_CLICK_AT_TARGET;
    }

    case 1:
        if (interface_mouse_over_panel())
            return WORLD_CLICK_NONE;
        *valid = world_pick_ground_point(cursor_spot_target_bm, cursor_spot_invalid_bm, l, face, hit, true);
        return WORLD_CLICK_AT_POINT;

    case 2:
        if (l && lobj_is_door(l)) {
            if (door_in_use_range(cur_character, l)) {
                world_hover_tint_character(l, world_hover_tint[0], world_hover_tint[1], world_hover_tint[2]);
                world_interact_object = l;
                return WORLD_CLICK_PICK_LOCK;
            }
            world_hover_tint_character(l, world_tint_enemy[0], world_tint_enemy[1], world_tint_enemy[2]);
            return WORLD_CLICK_NONE;
        }
        if (!picked || picked->type != 11)
            return WORLD_CLICK_NONE;
        {
            vector3 d = cur_character->pos - picked->pos;
            if (d.len_sq() <= 9.0f) {
                world_hover_tint_object(picked, world_hover_tint[0], world_hover_tint[1], world_hover_tint[2]);
                world_hover_object = picked;
                return WORLD_CLICK_PICK_LOCK;
            }
            world_hover_tint_object(picked, world_tint_enemy[0], world_tint_enemy[1], world_tint_enemy[2]);
        }
        break;
    }
    return WORLD_CLICK_NONE;
}

// D8902: by-value inline float min / max (as the camera unit, D7500); names ours.
static inline float pick_min(float a, float b) { return a < b ? a : b; }
static inline float pick_max(float a, float b) { return a > b ? a : b; }

// 0x43bba0  Confidence: exact (VC6 byte match, match30; was likely; D8902: by-value inline float min / max, the eye distance unnamed, the lobj index local). Earlier note: (the else-first order of the validity test; min / max of the decal scale).
// Places the target decal at pos, sized by the view (tan of the smaller fov times the eye distance) and,
// with check, valid only on a lobj with flag 0x40, navigable, within 5 m of height and 3..13 m away
// horizontally, on a surface the pick filter allows and in sight. The decal is turned towards the script
// editor's override point.
bool world_pick_ground_point(int ok_cursor, int bad_cursor, lobj *l, int face, vector3 *pos, bool check)
{
    float sy, sx;
    float h_fov, v_fov;
    float size;
    vector3 toward;
    vector3 eye_pos;
    matrix eye_orient;
    float angle;
    float dist_sq;
    bool valid;

    if (!world_pick_filter)
        world_pick_filter = world_pick_default_filter;
    gr_get_eye(&eye_pos, &eye_orient);
    gr_get_view_fov(&h_fov, &v_fov);
    float fov = pick_min(h_fov, v_fov);
    size = (float)tan(fov * 0.017453292f) * (eye_pos - *pos).len() * 0.2f;

    if (check) {
        float dx = pos->x - cur_character->pos.x;
        float dz = pos->z - cur_character->pos.z;
        dist_sq = dz * dz + dx * dx;
        if (!(l->flags & 0x40) || !pf_point_is_navigable(pos, 2, l->pick_layer(pos, 2))
            || !(fabsf(cur_character->pos.y - pos->y) < 5.0f) || !(dist_sq < 169.0f) || dist_sq < 9.0f)
            valid = false;
        else
            valid = true;
    } else {
        valid = true;
    }

    if (ui_get_pick_world_override(&toward)) {
        toward = vector3(pos->x - toward.x, 0.0f, pos->z - toward.z);
        toward.normalize_safe();
        angle = (float)acos(toward.x);
        if (toward.z < 0.0f)
            angle = 6.2831855f - angle;
    } else {
        angle = 0.0f;
    }

    if (check && valid) {
        int index = l - lobjs;
        if (!world_pick_filter[lobj_face_get_surface(index, face)]
            || !cur_character->has_line_of_sight_to_point(pos))
            valid = false;
    }

    if (!ui_get_pick_screen_override(&sx, &sy)) {
        sy = 1.0f;
        sx = 1.0f;
    }
    float scale = pick_min(size / sx, size / sy);
    scale = pick_min(pick_max(scale, 0.05f), 1.0f);
    place_ground_decal(valid ? ok_cursor : bad_cursor, pos, angle * 57.295776f, scale * sx, scale * sy);
    return valid;
}

// 0x43be80  Confidence: exact (the flag form of the navigability result). A click on the ground: no
// move in a conversation (game flag 0x100) or while an attack must finish; a point off the nav mesh is
// moved to the nearest navigable one, 1.5 m back towards the character; an avoidance obstacle refuses
// the click with the bad sound.
void world_click_move(unsigned int *input, vector3 *pos, lobj *l)
{
    if (!(input[0] & INPUT_CLICK) || (game_flags & 0x100))
        return;
    if ((cur_character->ai.flags & 2) && !cur_character->ai.is_combat_e_or_sneak_attack()) {
        game_play_sound(SOUND_CLICK_BAD, 1.0f, 0, 0, 0);
        return;
    }

    int layer = l->pick_layer(pos, 1);
    bool ok = true;
    if (cur_character->find_overlap(pos, NULL, 0))
        ok = false;
    if (!pf_point_is_navigable(pos, cur_character->info->size, layer)) {
        pf_find_nearest_navigable_point(pos, cur_character->info->size, layer);
        vector3 back = cur_character->pos - *pos;
        back.normalize();
        *pos += back * 1.5f;
        if (!pf_point_is_navigable(pos, cur_character->info->size, layer))
            ok = false;
    }
    if (avoid_point_blocked(pos, layer, cur_character->info->size)) {
        game_play_sound(SOUND_CLICK_BAD, 1.0f, 0, 0, 0);
        return;
    }
    if (!ok)
        return;

    world_click_handled = true;
    if (!(game_flags & 0x100)) {
        cur_character->ai_add_goal9(pos, layer, DIRECT_ORDER, (input[0] >> 3) & 1);
        game_play_sound(SOUND_CLICK_OK, 1.0f, 0, 0, 0);
    }
    world_last_click_pos = *pos;
}

// 0x43c060  Confidence: likely (the key search's flag; the duplicated message tails are the compiler's).
// A key (party inventory or equipped by a party member) unlocks a locked door for good ("Unlocked!").
// A client then sends service request 4 / 5 (closed / open) when unlocked, else only shows the message;
// single player and the host toggle the door and the host tells the others (door_find_by_mesh).
void object_try_unlock_or_open(lobj *l)
{
    bool is_door = lobj_is_door(l);
    bool open;
    bool locked;
    ai_service_req req;

    savegame_state.two_stage_get(&open, &locked, l);
    door_info *d = door_from_lobj(l);
    if (locked && d->key) {
        bool has_key = party_inventory_find_type(d->key) >= 0;
        if (!has_key) {
            for (Player *p = party_list.next; p != &party_list; p = p->next) {
                if (character_find_equipped_type(living_from_handle(p->handle), d->key) >= 0) {
                    has_key = true;
                    break;
                }
            }
        }
        if (has_key && locked) {
            savegame_state.two_stage_set(open, false, l);
            hud_show_message(XSTR("Unlocked!", 623));
            sound_play_2d(6, 1.0f);
            locked = false;
        }
    }

    if (MP_IS_CLIENT()) {
        if (!locked) {
            req.type = 4 + (open != 0);
            req._unk08 = l - lobjs;
            ai_service_request(&req);
            return;
        }
        if (is_door) {
            if (d->lock == 0.0f) {
                if (!d->key) {
                    hud_show_message(XSTR("Cannot be opened from here", 625));
                    sound_play_2d(5, 1.0f);
                    return;
                }
                hud_show_message(XSTR("Locked, Cannot Pick", 624));
                feedback_play(cur_character->info->species, 3, &cur_character->pos);
            } else {
                feedback_play(cur_character->info->species, 3, &cur_character->pos);
                hud_show_message(XSTR("Locked", 626));
            }
            sound_play_2d(5, 1.0f);
            return;
        }
        if (d->key) {
            hud_show_message(XSTR("Need item to use", 627));
            sound_play_2d(5, 1.0f);
            return;
        }
        hud_show_message(XSTR("Can't use", 628));
        sound_play_2d(SOUND_CLICK_BAD, 1.0f);
        return;
    }

    if (door_toggle_state(l, 0)) {
        if (MP_IS_HOST())
            door_find_by_mesh(mp_local_player->id, l - lobjs, !open);
        return;
    }
    if (is_door) {
        if (d->lock == 0.0f) {
            if (!d->key) {
                if (lobj_door_is_open(l, 1.0f)) {
                    hud_show_message(XSTR("Cannot be closed", 629));
                    sound_play_2d(5, 1.0f);
                    return;
                }
                hud_show_message(XSTR("Cannot be opened", 630));
                sound_play_2d(5, 1.0f);
                return;
            }
            hud_show_message(XSTR("Locked, Cannot Pick", 624));
            feedback_play(cur_character->info->species, 3, &cur_character->pos);
        } else {
            feedback_play(cur_character->info->species, 3, &cur_character->pos);
            hud_show_message(XSTR("Locked", 626));
        }
        sound_play_2d(5, 1.0f);
        return;
    }
    if (d->key) {
        hud_show_message(XSTR("Need item to use", 627));
        sound_play_2d(5, 1.0f);
        return;
    }
    hud_show_message(XSTR("Can't use", 628));
    sound_play_2d(SOUND_CLICK_BAD, 1.0f);
}

// 0x43c420  Confidence: likely (text is 0xfc bytes in the frame). A level container (lc entry): once
// its item or gold; an emptied one says "Nothing!", one without contents shows its name text.
void container_examine(lobj *l)
{
    char *buttons[1];
    char text[252];

    buttons[0] = XSTR("Ok", 213);
    int i = lc_find_by_name(l->name);
    level_script_use(l->name);
    if (lc_entries[i].gold > 0) {
        if (lc_entries[i].flags & 1) {
            ui_message_box_show(XSTR("Examine", 631), XSTR("Nothing!", 632), -1, -1, 1, buttons, NULL, 1);
            return;
        }
        lc_entry_mark_used(i);
        if (stricmp(lc_entries[i].name, "gold"))
            party_inventory_add_by_name(lc_entries[i].name, lc_entries[i].gold, -1, 1);
        else
            party_gold += lc_entries[i].gold;
        if (lc_entries[i].gold > 1)
            sprintf(text, XSTR("Found %d %s!", 633), lc_entries[i].gold, localize_item_name(lc_entries[i].name));
        else
            sprintf(text, XSTR("Found %s!", 634), localize_item_name(lc_entries[i].name));
        ui_message_box_show(XSTR("Examine", 631), text, -1, -1, 1, buttons, NULL, 1);
        return;
    }
    if (strlen(lc_entries[i].name))
        ui_message_box_show(XSTR("Examine", 631), lc_entries[i].name, -1, -1, 1, buttons, NULL, 1);
}

// 0x43c5a0  Confidence: exact. Starts the NPC's conversation (game flag 0x100 for its duration).
void player_talk_to(Character *npc)
{
    if (!game_is_paused()) {
        char *name = npc->get_name();
        if (!name || !*name)
            name = (char *)npc->info->name;
        if (conversation_db.has_speaker(name)) {
            game_play_sound(SOUND_CLICK_OK, 1.0f, 0, 0, 0);
            game_flags |= 0x100;
            conversation_begin(name, npc);
            return;
        }
    }
    game_play_sound(SOUND_CLICK_BAD, 1.0f, 0, 0, 0);
}

// 0x43c620  Confidence: likely (the order of the squared terms). The active, visible living entity, item
// or container nearest to the segment from the camera to the picked point: within its radius (an item's
// mesh sphere, 2 for a container, twice the character radius), horizontally within half of it (compared
// squared against the unsquared value: a quirk), the nearest, and replacing an earlier one only when at
// least half that one's radius away from it.
obj *pick_test_character(vector3 *hit)
{
    obj *best = NULL;
    float best_d = 10000000.0f;
    float best_r = 0.0f;
    float r;

    for (obj *o = obj_active_list.next; o != &obj_active_list; o = o->next) {
        if (!o->is_living() && o->type != 3 && o->type != 11)
            continue;
        if (!(o->obj_flags & 1) || !(o->obj_flags & 2))
            continue;

        vector3 off;
        if (o->type == 3) {
            off = ((item_obj *)o)->item->type->mesh->center;
            r = ((item_obj *)o)->item->type->mesh->radius;
        } else if (o->type == 11) {
            r = 2.0f;
            off.x = 0.0f;
            off.y = 0.0f;
            off.z = 0.0f;
        } else {
            r = ((Character *)o)->size_radius() * 2.0f;
            off.x = 0.0f;
            off.y = ((Character *)o)->size_height() * 0.5f;
            off.z = 0.0f;
        }
        off += o->pos;

        vector3 closest;
        ix_point_linesegment(&off, &camera_view_pos, hit, &closest);
        float dx = off.x - closest.x;
        float dy = off.y - closest.y;
        float dz = off.z - closest.z;
        float dz2 = dz * dz;
        float dx2 = dx * dx;
        float d2 = dy * dy + dz2 + dx2;
        if (r * r > d2) {
            r *= 0.5f;
            if (dz2 + dx2 < r && d2 < best_d
                && (!best || !(distance(&o->pos, &best->pos) * 0.5f < best_r))) {
                best_d = d2;
                best = o;
                best_r = r;
            }
        }
    }
    return best;
}

// 0x43c830  Confidence: exact (the three float tests after the messages are kept as written; -100 and
// 100 are chain_check_window's "not in a chain" results). On a right click outside the 1 s timer: the
// chain slot from the held key (Shift 1, Ctrl 2, Alt 3, CapsLock 0, none: the leader's default, which then
// cycles); in the window "Chain <skill>!" and the count, else "Too slow!" / "Too fast!". The chain marker
// is drawn while skill 0x55's window is open.
void chain_attack_select(unsigned int *input)
{
    char text[0x50];
    float w;

    if ((input[0] & INPUT_RCLICK) && (chain_attack_timer.elapsed() || !chain_attack_timer.valid())) {
        int is_default = 0;
        int slot;
        if (key_is_down(KEY_LSHIFT) || key_is_down(KEY_RSHIFT))
            slot = 1;
        else if (key_is_down(KEY_LCONTROL) || key_is_down(KEY_RCONTROL))
            slot = 2;
        else if (key_is_down(KEY_LALT) || key_is_down(KEY_RALT))
            slot = 3;
        else if (key_is_down(KEY_CAPSLOCK))
            slot = 0;
        else {
            is_default = 1;
            slot = party_leader->default_chain;
        }

        if (character_get_chain_skill(party_leader, slot) != SKILL_NONE) {
            chain_attack_timer.SetTimeout(1000);
            w = cur_character->combat.chain_check_window(character_get_chain_skill(party_leader, slot));
            if (w == 0.0f) {
                strcpy(text, XSTR("Chain ", 215));
                strcat(text, localize_string_or_invalid(skill_table[character_get_chain_skill(party_leader, slot)].name));
                strcat(text, "!");
                hud_show_message(text);
                sprintf(text, XSTR("Chain %d", 635), cur_character->combat.chain_count);
                hud_show_chain_message(text);
                sound_play_2d(3, 1.0f);
            } else if (w < 0.0f && w != -100.0f) {
                hud_show_chain_message(XSTR("Too slow!", 636));
                sound_play_2d(4, 1.0f);
            } else if (w > 0.0f) {
                hud_show_chain_message(XSTR("Too fast!", 637));
                sound_play_2d(4, 1.0f);
            }
            if ((w == 0.0f || (w < 0.0f && w != -100.0f) || (w > 0.0f && w != 100.0f)) && is_default)
                character_cycle_default_chain(party_leader);
            if (w != 0.0f && w != -100.0f && w != 100.0f)
                return;
        }
    }
    if (cur_character->combat.chain_check_window(SKILL_NONE) == 0.0f)
        levelrender_draw_chain_marker = true;
}

// 0x43cb60  Confidence: exact (jump table 0x43cd30 for kinds 1..4). The action hud_interact_nearest chose
// (the interact key): 1 gold pile, 2 container object, 3 door, 4 level container.
void object_use_container()
{
    ai_service_req req;
    char text[128];
    obj *o;

    switch (world_interact_kind) {
    case 1:
        if (MP_IS_CLIENT()) {
            req.type = 2;
            req._unk04 = ((obj *)world_hover_object)->net_handle;
            ai_service_request(&req);
            break;
        }
        if (MP_IS_HOST())
            mp_broadcast_container_gold(mp_local_player->id, ((obj *)world_hover_object)->handle);
        o = (obj *)world_hover_object;
        sprintf(text, XSTR("Picked up %d Gold", 223), ((item_obj *)o)->count);
        hud_show_message(text);
        item_obj_pickup((item_obj *)o, 1);
        break;
    case 2:
        o = (obj *)world_hover_object;
        if (o->obj_flags & 4)
            break;
        if ((game_flags & GF_MULTIPLAYER) && ((container *)o)->user != -1) {
            hud_show_message(XSTR("Container is currently in use", 638));
            break;
        }
        if (((container *)o)->lock > 0.0f) {
            hud_show_message(XSTR("Locked", 626));
            feedback_play(cur_character->info->species, 3, &cur_character->pos);
            sound_play_2d(5, 1.0f);
            break;
        }
        object_open_pickup(o);
        break;
    case 3:
        object_try_unlock_or_open((lobj *)world_interact_object);
        break;
    case 4:
        container_examine((lobj *)world_interact_object);
        break;
    }
    world_interact_kind = 0;
}

// 0x43cd50  Confidence: exact. monster_drop_loot passes the dropped item's handle.
void hud_set_timed_highlight(int handle)
{
    if (handle != -1) {
        gold_highlight_handle = handle;
        gold_highlight_timer.SetTimeout(5000);
    }
}
