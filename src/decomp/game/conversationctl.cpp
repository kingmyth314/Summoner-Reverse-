// conversationctl.cpp - Sum.exe v1.40, code 0x4d87d0-0x4db249 (the conversation controller)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// The running conversation conversation_active (0x25464b0, class conversation_controller): walking the
// player up to the speaker, turning both, the conversation camera (a shot over the player's shoulder, blended
// in and out), the talk animations of whoever speaks the lines on screen, topic selection and the end
// (shop or back to the game). File name from strings.tbl id 468 ("<Quest Journal Updated>", read by
// conversation_select_topic). Unit range: static_init_conversation_controller_25464b0 (0x4d87d0, initialiser
// table entry 370) to cmd_dcamera's last RET (0x4db248); str_copy_append_32 (0x4db250) starts the next unit.
// The three console commands (entries 371-373, 0x4dafe0-0x4db248) are untagged in sum.symbols; they are
// here because their objects follow conversation_active in .bss (0x2546730-0x254675f, before the next
// unit's cutscene_saved_equipment 0x2546760) and their strings follow conversation.cpp's in .data
// (0x5a02cc-0x5a0451), and they only call conversation_db.
//
// Header inlines: 0x4d8f30 game_camera__get_pos, 0x4d8f40 game_camera__get_orient, 0x4d9a80
// vector3__len_sq and 0x4da830 character_get_center are COMDATs of inline header functions (camera.h,
// vector.h, living_entity.h); 0x4d8f00 dialog_camera_pose__set and 0x4d9aa0 dialog_camera_pose__assign
// likewise (conversation.h, DECISIONS.md C3). Their bodies are in the headers; this file marks the copies
// at their addresses (as gr_direct3d.cpp does for gr::Mode).
//
// vector3 arithmetic: the binary expands some vector3 operators inline and calls the out-of-line copies
// (0x45b680 +, 0x45b6b0 -, 0x413190 =, 0x468a90 float * v) elsewhere, in the same function. That is VC6
// declining to expand inline members at some call sites; the source writes the operators throughout.

#include <string.h>
#include <math.h>
#include <malloc.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/console.h"
#include "vsdk/gr/gr.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/matrix.h"
#include "engine/objects/living_entity.h"
#include "engine/camera/camera.h"
#include "engine/ai/pathfinding.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_buysell.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/conversation.h"

#define PI 3.1415927f

// ---- .bss ----------------------------------------------------------------------------------------
// 0x4d87d0 static_init_conversation_controller_25464b0 (call 0x4d87e0; jmp 0x4d87f0) /
// 0x4d87e0 init_conversation_controller_25464b0 (mov ecx, 0x25464b0; jmp conversation_controller__ctor) /
// 0x4d87f0 register_atexit_conversation_controller_25464b0 /
// 0x4d8800 atexit_dtor_conversation_controller_25464b0
// (jmp 0x4d8810) are the compiler's helpers of this definition.
// 0x4d8810 conversation_controller__dtor is
// the implicit destructor: `eh vector destructor iterator` over slots[16], then the two list heads'
// ~conversation_slot (both folded into empty_function). Confidence: exact.
conversation_controller conversation_active;          // 0x25464b0 (tagged living_entity.cpp in sum.symbols)

// 0x254672c: pf_find_nearest_navigable_point's result for the walk-up point, written and read only by
// conversation_frame. Absolute address, right after the controller's 0x27c bytes, so a separate variable
// (DECISIONS.md C4). It could also be a function-local static of conversation_frame. Name ours.
static bool conversation_dest_found;

// 0x4d8880  Confidence: exact. NULL when all 16 slots are in use.
conversation_slot *conversation_controller::add_talker(Character *c, int kind)
{
    conversation_slot *s = free_slots.next;

    if (s == &free_slots)
        return NULL;
    list_remove(s);
    s->reset();
    s->character = c;
    s->kind = kind;
    list_append(&active, s);
    return s;
}

// 0x4d8900  Confidence: exact. Only quoted text is spoken (a talk animation is played).
conversation_slot *conversation_controller::topic_talker(conversation_topic *topic)
{
    const char *text = topic->text;
    if (!strchr(text, '"'))
        return NULL;
    return add_talker(cur_character, 0);
}

// 0x4d8940  Confidence: likely (marked exact earlier; differs under cl 12.00.8966, match31) (switch through the table at 0x4d8a6c). The party lines go to the first
// living entity of that species.
conversation_slot *conversation_controller::line_talker(conversation_line *line)
{
    Character *c;

    if (!strchr(line->text, '"'))
        return NULL;
    switch (line->kind) {
    case CLINE_PC:
        return add_talker(cur_character, 1);
    case CLINE_NPC:
        return add_talker(speaker, 1);
    case CLINE_JOSEPH:
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->info->species == SPECIES_JOSEPH)
                return add_talker(c, 1);
        }
        break;
    case CLINE_ROSALIND:
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->info->species == SPECIES_ROSALIND)
                return add_talker(c, 1);
        }
        break;
    case CLINE_FLECE:
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->info->species == SPECIES_FLECE)
                return add_talker(c, 1);
        }
        break;
    case CLINE_JEKHAR:
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->info->species == SPECIES_JEKHAR)
                return add_talker(c, 1);
        }
        break;
    }
    return NULL;
}

// 0x4d8a90  Confidence: exact (the loops match; `(first + i) % 64` is written in the operand order of
// each loop's lea). The first talker is retired when its move ended; every talker of a line up to the last
// visible one starts talking; the talkers before the one of the first visible line are retired when they
// play nothing; the first talker gets a new talk move when it has none.
void conversation_controller::update_talk_anims()
{
    conversation_slot *talker = NULL;
    conversation_slot *s, *next;
    dialogue_line *line;
    int i, idx, last, first;

    s = active.next;
    if (s != &active && s->talking && s->character->move_finished(s->move)) {
        list_remove(s);
        list_append(&free_slots, s);
    }

    last = dialogue_text_box.last_visible_line();
    for (i = 0; i < dialogue_text_box.count; i++) {
        idx = (dialogue_text_box.first + i) % 64;
        line = &dialogue_text_box.line[idx];
        if ((line->kind == 0 || line->kind == 2) && line->talker && !line->talker->talking)
            line->talker->talking = true;
        if (idx == last)
            break;
    }

    first = dialogue_text_box.first_visible_line();
    for (i = 0; i < dialogue_text_box.count; i++) {
        idx = (i + dialogue_text_box.first) % 64;
        line = &dialogue_text_box.line[idx];
        if ((line->kind == 0 || line->kind == 2) && line->talker)
            talker = line->talker;
        if (idx == first)
            break;
    }

    for (s = active.next; s != &active; s = s->next) {
        if (s == talker)
            break;
    }
    if (s != &active && talker && talker != active.next && talker != active.next->next) {
        for (s = active.next, next = s->next; s != &active && s != talker && s->talking;
             s = next, next = next->next) {
            if (s->move < 0) {
                list_remove(s);
                list_append(&free_slots, s);
            }
        }
    }

    s = active.next;
    if (s != &active && s->talking && s->move < 0) {
        int move = s->character->pick_talk_move();
        if (move)
            s->move = s->character->play_move(move, 1.0f, 0, 0, 0);
    }
}

// 0x4d8ca0  Confidence: exact (prev is cleared before next).
conversation_slot::conversation_slot()
{
    prev = NULL;
    next = NULL;
    reset();
}

// Empty. Its code is the shared RET empty_function (0x4f13e0): identical-code folding removed this copy,
// so its place in the unit is not known. The controller's destructor and the `eh vector` iterators of
// the constructor call it. Confidence: likely.
conversation_slot::~conversation_slot()
{
}

// 0x4d8cc0  Confidence: exact.
void conversation_slot::reset()
{
    character = NULL;
    move = -1;
    talking = false;
    kind = -1;
}

// 0x4d8ce0  Confidence: exact (members built in order: the two GameTimers, the heads, the slot array
// through `eh vector constructor iterator`; then the body).
conversation_controller::conversation_controller()
{
    int i;

    state = 0;
    list_init(&active);
    list_init(&free_slots);
    for (i = 0; i < 16; i++)
        list_append(&free_slots, &slots[i]);
}

// 0x4d8db0  Confidence: exact (VC6 byte match, match9; was likely: state is stored between the flag load and store).
void conversation_controller::open_dialogue()
{
    char name[256];

    speaker->get_display_name(name);
    dialogue_screen_open(name);
    state = 1;
    busy = 1;
    select_topic(conversation_db.select_greeting());
    busy = 0;
}

// The point transform of an inline matrix43 operator of the original math header (name and form not
// known; matrix43::transform 0x511c70 is the out-of-line one). Expanded in conversation_camera_capture.
static inline vector3 conversation_transform(const matrix43 &m, const vector3 &v)
{
    vector3 r;
    r.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    r.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    r.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    return r;
}

// 0x4d8e10  Confidence: likely (the transform helper above is a guess at the source form).
// The game camera's eye and forward this frame; it is the blend start unless the shot changed sides
// (only while moving out).
void conversation_controller::camera_capture()
{
    vector3 fwd(0.0f, 0.0f, 1.0f);

    game.set(game_camera.get_pos(), conversation_transform(matrix43(game_camera.get_orient()), fwd));
    if (!shot_cut || cam_mode == 3)
        start = game;
}

// 0x4d8f00 dialog_camera_pose__set: COMDAT of the inline dialog_camera_pose::set(vector3, vector3)
// (levelscripts/conversation.h), six dword stores, ret 0x18. Confidence: exact.
// 0x4d8f30 game_camera__get_pos: COMDAT of camera::get_pos() (engine/camera/camera.h), lea eax,
// [ecx+0x1a4]. Confidence: exact.
// 0x4d8f40 game_camera__get_orient: COMDAT of camera::get_orient(), lea eax, [ecx+0x1b0]. Confidence:
// exact.

// 0x4d8f50  Confidence: unsure (the arithmetic follows the binary; which operators were expanded inline,
// the operand order of the commutative sums and the shared tails are the compiler's or guesses).
// Two camera points beside the player, on either side of the line between the two characters; the one
// nearer the current view is used, and kept until the other side is chosen while holding. The speaker's
// char_info flags 0x1000 / 0x2000 / 0x4000 select wider shots for big creatures.
// Game bug kept: side.y is never set (the binary writes only +0x54 and +0x5c of the vector), so its
// normalisation reads stack garbage.
void conversation_controller::camera_compute_shot()
{
    vector3 speaker_centre = speaker->pos + speaker->centre_offset;
    vector3 player_centre = cur_character->pos + cur_character->centre_offset;
    vector3 d = speaker_centre - player_centre;
    vector3 side;
    vector3 look, pos_a, pos_b, chosen;
    unsigned int f;
    int side_b;

    side.z = -d.x;
    d.y = 0.0f;
    side.x = d.z;
    d.normalize_safe();
    side.normalize_safe();

    f = speaker->info->flags;
    if (f & 0x1000) {
        look = player_centre * 0.25f + speaker_centre * 0.75f;
        look.y += speaker->size_height() * 0.75f + cur_character->size_height() * 0.25f;
        pos_a = player_centre - d + side;
        pos_b = player_centre - d - side;
        pos_a.y = cur_character->size_height() + cur_character->pos.y;
        pos_b.y = cur_character->size_height() + cur_character->pos.y;
    } else if (f & 0x2000) {
        look = player_centre * 0.25f + speaker_centre * 0.75f;
        look.y = cur_character->size_height() + look.y + 0.5f;
        pos_a = side * 5.0f + (player_centre + d);
        pos_b = (player_centre + d) - side * 5.0f;
        pos_a.y = cur_character->size_height() * 2 + cur_character->pos.y;
        pos_b.y = cur_character->size_height() * 2 + cur_character->pos.y;
    } else if (f & 0x4000) {
        look = player_centre * 0.25f + speaker_centre * 0.75f;
        look.y = cur_character->size_height() + look.y + 0.25f;
        pos_a = side * 0.5f + (player_centre - d);
        pos_b = (player_centre - d) - side * 0.5f;
        pos_a.y = cur_character->size_height() + cur_character->pos.y;
        pos_b.y = cur_character->size_height() + cur_character->pos.y;
    } else {
        look = player_centre * 0.25f + speaker_centre * 0.75f;
        look.y += speaker->size_height() * 0.75f + cur_character->size_height() * 0.25f;
        pos_a = side * 0.5f + (player_centre - d);
        pos_b = player_centre - d - 0.5f * side;
        pos_a.y = cur_character->size_height() + cur_character->pos.y;
        pos_b.y = cur_character->size_height() + cur_character->pos.y;
    }

    if (cam_mode != 1 && shot_valid) {
        side_b = shot_side;
        if (side_b)
            chosen = pos_b;
        else
            chosen = pos_a;
    } else {
        float dist_a = (pos_a - camera_view_pos).len_sq();
        if ((pos_b - camera_view_pos).len_sq() < dist_a) {
            side_b = 1;
            chosen = pos_b;
        } else {
            side_b = 0;
            chosen = pos_a;
        }
    }

    if (!shot_valid) {
        shot_side = side_b;
        shot_valid = 1;
    } else {
        if (side_b)
            chosen = pos_b;
        else
            chosen = pos_a;
        if (side_b != (int)shot_side) {
            shot_side = side_b;
            shot_cut = 1;
            out.pos - chosen;    // computed and not used: the out-of-line operator- call stays in the binary
            if (!cam_timer.elapsed()) {
                start = out;
                move_in_time = (float)cam_timer.GetRemainingTime();
            } else {
                start = game;
            }
        }
    }
    shot.pos = chosen;
    shot.dir = look - chosen;
}

// 0x4d9a80 vector3__len_sq: COMDAT of vector3::len_sq() (vsdk/math/vector.h), x*x + y*y + z*z.
// Confidence: exact.
// 0x4d9aa0 dialog_camera_pose__assign: COMDAT of dialog_camera_pose::operator= (conversation.h), six
// dword copies, returns this. Confidence: exact.

// 0x4d9ad0  Confidence: likely (thiscall, `this` not used). out = a * t + b * (1 - t) with t eased
// (2t^2 below one half, 4t - 2t^2 - 1 above); the direction is interpolated as axis and angle, the axis of
// b flipped to a's side and the angles kept less than pi apart.
void conversation_controller::camera_blend(dialog_camera_pose *out, dialog_camera_pose *a,
                                           dialog_camera_pose *b, float t)
{
    matrix m, mb;
    matrix43 m43;
    vector3 axis_a, axis_b, axis, fwd;
    float angle_a, angle_b, angle, u;

    if (t > 0.5f)
        t = t * 4.0f - t * t * 2.0f - 1.0f;
    else
        t = t * t * 2.0f;

    m.set_from_forward(a->dir);
    m.get_axis_angle(&axis_a, &angle_a);
    mb.set_from_forward(b->dir);
    mb.get_axis_angle(&axis_b, &angle_b);

    if (axis_a.z * axis_b.z + axis_a.y * axis_b.y + axis_b.x * axis_a.x < 0.0f) {
        axis_b = axis_b * -1.0f;
        angle_b = -angle_b;
    }
    if (fabs(angle_a - angle_b) > PI) {
        if (angle_b > angle_a)
            angle_b -= 2 * PI;
        else
            angle_b += 2 * PI;
    }

    u = 1.0f - t;
    axis = axis_a * t + axis_b * u;
    angle = angle_a * t + angle_b * u;
    m.set_from_axis_angle(axis, angle);
    m43 = matrix43(m);
    fwd.x = m43.m[3][0] + m43.m[2][0];
    fwd.y = m43.m[3][1] + m43.m[2][1];
    fwd.z = m43.m[3][2] + m43.m[2][2];
    out->dir = fwd;
    out->pos = t * a->pos + u * b->pos;
}

// 0x4d9d90  Confidence: likely (the `cam_mode == 3` test of the no-zoom branch is reached through two
// earlier compares in the binary: a switch with empty cases 1 and 0 is assumed; the copies of a pose are
// member by member here, the pose assignment elsewhere). Returns -1 when the camera is back and the
// conversation is over.
int conversation_controller::camera_update()
{
    int result = 0;
    int mode;
    float t;

    camera_capture();
    if ((speaker_flags & CFLAG_NO_CAMERA_MOVEMENT) || Summoner_options.dialogue_camera == 1
        || (Summoner_options.dialogue_camera == 2 && !(speaker_flags & CFLAG_IMPORTANT))) {
        // No zoom: the view stays the game camera's.
        out.pos = game.pos;
        out.dir = game.dir;
        switch (cam_mode) {
        case 1:
        case 0:
            break;
        case 3:
            game_camera.pop_state();
            game_camera.resume_follow();
            state = 0;
            result = -1;
            break;
        }
    } else if (cam_mode == 0) {
        out.pos = game.pos;
        out.dir = game.dir;
    } else if (cam_mode == 2) {
        out.pos = shot.pos;
        out.dir = shot.dir;
    } else {
        mode = cam_mode;
        if (mode != 3)
            camera_compute_shot();
        if (cam_mode == 1 || cam_mode == 3) {
            float total = (cam_mode == 1) ? move_in_time : move_out_time;
            t = 1.0f - cam_timer.GetRemainingTime() / total;
            if (t >= 1.0f) {
                t = 1.0f;
                if (cam_mode == 1) {
                    cam_mode = 2;
                } else {
                    game_camera.pop_state();
                    game_camera.resume_follow();
                    state = 0;
                    result = -1;
                }
            }
            t = (mode == 1) ? t : 1.0f - t;
            camera_blend(&blend, &shot, &start, t);
        } else {
            blend.pos = shot.pos;
            blend.dir = shot.dir;
        }
        out.pos = blend.pos;
        out.dir = blend.dir;
    }
    camera_view_pos = out.pos;
    camera_view_orient.set_from_forward(out.dir);
    return result;
}

// 0x4da010  Confidence: likely (the switch, the gotos to the shared give-up code and the expanded
// get_center / len_sq are written to the binary's blocks; the operand order of the sums is a guess).
// Returns -1 when no conversation runs, the camera's -1 when it ended, else 0.
int conversation_controller::frame()
{
    float dist;
    int other_layer;
    int r;

    if (state == 0 && cam_mode == 0)
        return -1;
    r = camera_update();
    if (r != 0)
        return r;
    if (cam_mode == 3)
        return 0;

    dist = cur_character->distance_between_edges(speaker, false, -1);
    other_layer = cur_character->layer != speaker->layer;
    if (cam_mode == 0 && (dist <= 0.675f || sliding || (approach == 3 && facing == 1))) {
        cam_mode = 1;
        cam_timer.SetTimeout((int)move_in_time);
    }

    switch (approach) {
    case 0: {
        // Walk the player up to the speaker (or to where the speaker is heading).
        vector3 target, dir, dest;
        int layer;
        float radius;

        if (cur_character->ai.is_combat_e_or_sneak_attack())
            return 0;
        if (timer.elapsed()) {
            approach = 1;
            return 0;
        }
        if ((cur_character->flags & 0x1) && !(cur_character->flags & 0x100)) {
            combat_mode_set(0);
            return 0;
        }
        if (speaker->is_moving()) {
            float ahead = speaker->stopping_distance(NULL) * 0.5f;
            vector3 step = speaker->orient.fvec * ahead;
            target = speaker->pos + speaker->centre_offset + step;
        } else {
            target = speaker->pos + speaker->centre_offset;
        }
        dir = cur_character->pos + cur_character->centre_offset - target;
        dir.normalize_safe();
        radius = speaker->size_radius() + cur_character->size_radius() + 0.2f;
        dir = dir * radius;
        target = target + dir;
        dest = target;
        conversation_dest_found = pf_find_nearest_navigable_point(&dest, cur_character->info->size,
                                                                  speaker->layer);
        layer = speaker->layer;
        if (other_layer) {
            ai_path *path = ai_path_find(&cur_character->pos, cur_character->layer, &dest, layer,
                                         cur_character->info->size, 0);
            if (path) {
                ai_path_free(path);
                layer = speaker->layer;
            } else {
                dest = target;
                pf_find_nearest_navigable_point(&dest, cur_character->info->size, cur_character->layer);
                layer = cur_character->layer;
            }
        }
        cur_character->ai.goals_clear();
        cur_character->ai.target = speaker->handle;
        if (distance(&cur_character->pos, &dest) < 0.75f
            && distance(&speaker->pos, &cur_character->pos) < distance(&speaker->pos, &dest)) {
            // Close, and the point is beyond the player: slide there instead of walking.
            sliding = 1;
            slide_x = dest.x;
            slide_z = dest.z;
            approach = 1;
            return 0;
        }
        cur_character->ai_add_goal9(&dest, layer, 999999, 0);
        approach = 1;
        return 0;
    }

    case 1:
        // Wait until the player has arrived (or slide the last bit).
        if (timer.elapsed())
            goto give_up;
        if (sliding) {
            vector2 d;
            vector3 move;
            float len, mx, mz;

            d.x = slide_x - cur_character->pos.x;
            d.y = slide_z - cur_character->pos.z;
            len = d.len();
            if (len < 0.05f) {
                mz = 0.0f;
                mx = 0.0f;
                sliding = 0;
            } else if (reported_frametime > len) {
                sliding = 0;
                mz = d.y;
                mx = d.x;
            } else {
                float s = reported_frametime / len;
                mx = s * d.x;
                mz = d.y * s;
            }
            move.x = mx;
            move.z = mz;
            move.y = 0.0f;
            cur_character->clip_move_to_navmesh(&move);
            cur_character->layer = pf_trace_edge_crossings(&cur_character->pos, cur_character->layer,
                                                           move + cur_character->pos);
            cur_character->pos.x += move.x;
            cur_character->pos.z += move.z;
            return 0;
        }
        if (!cur_character->ai.is_traveling_sub0()
            && cur_character->ai.goal_find(1, 9, 0, 0, -1, 0, 0, 0x6c) == -1) {
            if (!cur_character->is_moving() && !speaker->is_moving())
                goto give_up;
            if (dist <= 0.45f)
                goto give_up;
            if (other_layer && dist < 1.5f)
                goto give_up;
            if (!conversation_dest_found)
                approach = 0;
            return 0;
        }
        if (speaker->is_moving())
            return 0;
        if (dist > 0.45f)
            return 0;
        cur_character->ai.goals_clear();
        cur_character->ai.go_idle();
    give_up:
        cur_character->ai.goals_clear();
        cur_character->ai.go_idle();
        cur_character->walk_to_point(speaker->get_center(), 999999);
        approach = 2;
        return 0;

    case 2:
        // Turning to face the speaker.
        if (!character_has_ai_goal_10(cur_character))
            approach = 3;
        break;
    }

    if (facing == 0 && !speaker->is_moving() && !cur_character->is_moving()) {
        facing = 1;
        if (!(speaker_flags & CFLAG_DONT_FACE_PLAYER))
            speaker->walk_to_point(cur_character->get_center(), 999999);
    }
    if (state == 3 && approach == 3)
        open_dialogue();
    if (!(speaker_flags & CFLAG_DONT_FACE_PLAYER) && !character_has_ai_goal_10(speaker)) {
        // Keep facing the player when the player moves.
        vector3 c = cur_character->get_center();
        if ((player_pos - c).len_sq() >= 0.1f) {
            speaker->walk_to_point(cur_character->get_center(), 999999);
            player_pos = cur_character->get_center();
        }
    }
    if (state == 1 || state == 2)
        update_talk_anims();
    busy = 0;
    return 0;
}

// 0x4da830 character_get_center: COMDAT of Character::get_center() (engine/objects/living_entity.h), the
// position +0x10 plus the centre offset +0x28 into the hidden result. Confidence: exact.

// 0x4da860  Confidence: exact (cdecl; setne). Goal type 10 is the turn-to-face goal of walk_to_point (the
// type's name is not known).
bool character_has_ai_goal_10(Character *c)
{
    return c->ai.goal_find(1, 10, 0, 0, -1, 0, 0, 0x6c) != -1;
}

// 0x4da890  Confidence: likely (the order of the stores between the calls is partly the compiler's).
// Starts a conversation with `speaker` under the conversation file's name `name`: 0, or -1 when the
// file has no such speaker (or its hail topic is hidden).
int conversation_controller::camera_begin(const char *name, Character *character)
{
    conversation_slot *s;
    matrix43 m;
    vector3 fwd;

    if (conversation_db.set_speaker(name) != 0) {
        end_cleanup(false);
        return -1;
    }
    for (s = active.next; s != &active; s = active.next) {
        list_remove(s);
        list_append(&free_slots, s);
    }
    flag0 = 0;
    move_in_time = (float)conversation_db.speaker->move_in_time;
    move_out_time = (float)conversation_db.speaker->move_out_time;
    this->name = name;
    speaker_flags = conversation_db.speaker->flags;
    game_camera.push_state();
    game_camera.set_follow_flags(true, false);
    speaker = character;
    frame_height = (cur_character->size_height() + character->size_height()) * 0.5f + 0.25f;
    timer.SetTimeout(4000);
    state = 3;
    shot_cut = 0;
    sliding = 0;
    approach = 0;
    cam_mode = 0;
    facing = 0;
    player_pos = cur_character->get_center();
    if (character->script_object >= 0)
        script_object_suspend(character->script_object);
    m = matrix43(character->orient);
    fwd.x = m.m[3][0] + m.m[2][0];
    fwd.y = m.m[3][1] + m.m[2][1];
    fwd.z = m.m[3][2] + m.m[2][2];
    speaker_dir = fwd;
    character->ai.go_idle();
    shot_valid = 0;
    return 0;
}

// 0x4daa70  Confidence: exact (the two pose copies are expanded in place; the shop branch closes the
// dialogue screen itself and returns).
void conversation_controller::end_cleanup(bool open_shop)
{
    if (state == 0)
        return;
    speaker->ai.goals_clear();
    speaker->walk_to_point(speaker->get_center() + speaker_dir, 999999);
    if (speaker->script_object >= 0)
        script_object_resume_index(speaker->script_object);
    shot = out;
    start = game;
    cam_timer.SetTimeout((int)move_out_time);
    state = 0;
    cam_mode = 3;
    conversation_db.clear_speaker();
    if (open_shop && level_shopkeeper_find(speaker->get_name())) {
        dialogue_screen_close();
        shop_set_shopkeeper_from_char(speaker);
        interface_set_screen(7, NULL);
        return;
    }
    dialogue_screen_close();
}

// 0x4dabb0  Confidence: exact. From dialogue_frame (topic box click).
void conversation_controller::choose_topic(conversation_topic *topic)
{
    if (state == 1) {
        busy = 1;
        conversation_db.set_topic(topic);
        select_topic(topic);
        busy = 0;
    }
}

// 0x4dabf0  Confidence: exact (tail jump into conversation_end).
void conversation_controller::request_end()
{
    if (!busy) {
        busy = 1;
        if (state == 2)
            end();
    }
}

// 0x4dac10  Confidence: exact.
void conversation_controller::reset_state()
{
    state = 0;
}

// 0x4dac20  Confidence: exact.
bool conversation_controller::is_idle()
{
    return state == 0;
}

// 0x4dac30  Confidence: exact. The lines are over: farewell and shop topics end the conversation, else
// the remaining topics are offered.
void conversation_controller::end()
{
    state = 1;
    if (!(bool)conversation_db.topic->farewell) {
        if ((bool)conversation_db.topic->shop) {
            end_cleanup(true);
            return;
        }
        if (conversation_db.first_topic() != NULL) {
            dialogue_topic_box_reset();
            conversation_db.build_topic_list();
            conversation_db.farewell();
            if (dialogue_topic_box.count > 0)
                dialogue_selected_topic = 0;
            return;
        }
    }
    end_cleanup(false);

}

// 0x4dacb0  Confidence: likely (the clamp is imin(imax(..)): the setge / dec / and of the inline imin;
// the scroll search uses the clamped value kept in a register).
// Shows the topic's text (unless it is the greeting, said first) and its lines, runs the lines' actions
// and scrolls the text box to the topic text.
void conversation_controller::select_topic(conversation_topic *topic)
{
    conversation_line *line;
    dialogue_action *a;
    dialogue_line *l;
    int i, idx;

    if (topic == NULL || (topic->lines == NULL && (topic->farewell || topic->shop))) {
        end();
        return;
    }
    dialogue_topic_box_reset();
    state = 2;

    if (dialogue_text_box.count == 0) {
        for (line = topic->lines; line; line = line->next) {
            dialogue_show_line(line->text, 0, line_talker(line));
            for (a = line->actions; a; a = a->next)
                conversation_execute_action(a);
        }
        if (topic->hide_quests || topic->unhide_quests)
            dialogue_show_line(XSTR("<Quest Journal Updated>", 468), 1, NULL);
        return;
    }

    dialogue_show_line(topic->text, 2, topic_talker(topic));
    dialogue_text_box.scroll_target =
        -dialogue_text_box.line[(dialogue_text_box.first + dialogue_text_box.count - 1) % 64].top;
    for (line = topic->lines; line; line = line->next) {
        dialogue_show_line(line->text, 0, line_talker(line));
        for (a = line->actions; a; a = a->next)
            conversation_execute_action(a);
    }
    if (topic->hide_quests || topic->unhide_quests)
        dialogue_show_line(XSTR("<Quest Journal Updated>", 468), 1, NULL);

    dialogue_text_box.scroll_target = imin(imax(dialogue_text_box.scroll_target,
                                                gr_get_font_height(-1) * dialogue_text_box.lines
                                                - dialogue_text_box.text_height), 0);
    for (i = 0; i < dialogue_text_box.count; i++) {
        idx = (i + dialogue_text_box.first) % 64;
        l = &dialogue_text_box.line[idx];
        if (-dialogue_text_box.scroll_target >= l->top
            && -dialogue_text_box.scroll_target < l->top + l->height) {
            if (l->_unk14 == 0)
                dialogue_text_box.scroll_target = -dialogue_text_box.line[(idx + 1) % 64].top;
            return;
        }
    }
}

// 0x4daef0  Confidence: exact (the slot record's speaker count is read as an int through the void*
// table). From state_game_enter with (level_filename, level_script_name).
int conversation_controller::load_for_level(const char *level_name, const char *script_name)
{
    int slot;

    empty_function();
    conversation_db.clear();
    slot = level_script_find_index(cur_level_index, level_script_name);
    if ((int)conversation_save_table[(cur_level_index * 7 + slot) * 4 + 1] > 0)
        return conversation_db.load(level_name, script_name);
    return 0;
}

// 0x4daf50  Confidence: exact. cur_character stops (unless fighting) and loses status 8.
int conversation_begin(const char *name, Character *speaker)
{
    if (!cur_character->ai.is_combat_e_or_sneak_attack()) {
        cur_character->ai.goals_clear();
        cur_character->ai.go_idle();
    }
    if (cur_character->has_status(8)) {
        status_effect_update_fx(cur_character->handle, 6);
        cur_character->status_effect_cure(8, 1, 0);
    }
    return conversation_active.camera_begin(name, speaker);
}

// ---- console commands ----------------------------------------------------------------------------
// 0x4dafe0 static_init_ccmd_dialogue_make_topic_available (jmp) /
// 0x4daff0 init_ccmd_dialogue_make_topic_available: the dynamic
// initialiser. Confidence: exact.
console_command ccmd_dialogue_make_topic_available("dialogue_make_topic_available",
                                                   "mark a specific dialogue topic as available.",
                                                   cmd_dialogue_make_topic_available);   // 0x2546740

// 0x4db010  Confidence: exact (_alloca of the argument copy, frame pointer; the two usage prints are one
// call in the binary).
void cmd_dialogue_make_topic_available()
{
    if (calling_a_command_function) {
        char *buf, *speaker_name, *topic_name, *extra;

        process_command(ARG_STRING);
        buf = (char *)_alloca(strlen(next_arg) + 1);
        strcpy(buf, next_arg);
        speaker_name = strtok(buf, " \r\n\t");
        topic_name = strtok(NULL, " \r\n\t");
        extra = strtok(NULL, " \r\n\t");
        if (!speaker_name || !topic_name || extra) {
            print_to_console("Usage: dialogue_make_topic_available person topic", 0);
            return;
        }
        conversation_db.set_topic_available(speaker_name, topic_name, true);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: dialogue_make_topic_available person topic", 0);
}

// 0x4db0d0 static_init_ccmd_dialogue_make_topic_unavailable (jmp) /
// 0x4db0e0 init_ccmd_dialogue_make_topic_unavailable. Confidence: exact.
// (.bss order: this object is at 0x2546730, before the "available" one at 0x2546740.)
console_command ccmd_dialogue_make_topic_unavailable("dialogue_make_topic_unavailable",
                                                     "mark a specific dialogue topic as unavailable.",
                                                     cmd_dialogue_make_topic_unavailable);   // 0x2546730

// 0x4db100  Confidence: exact (as cmd_dialogue_make_topic_available; the false is the zero of the
// strtok test, `push eax`).
void cmd_dialogue_make_topic_unavailable()
{
    if (calling_a_command_function) {
        char *buf, *speaker_name, *topic_name, *extra;

        process_command(ARG_STRING);
        buf = (char *)_alloca(strlen(next_arg) + 1);
        strcpy(buf, next_arg);
        speaker_name = strtok(buf, " \r\n\t");
        topic_name = strtok(NULL, " \r\n\t");
        extra = strtok(NULL, " \r\n\t");
        if (!speaker_name || !topic_name || extra) {
            print_to_console("Usage: dialogue_make_topic_unavailable person topic", 0);
            return;
        }
        conversation_db.set_topic_available(speaker_name, topic_name, false);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: dialogue_make_topic_unavailable person topic", 0);
}

// 0x4db1c0 static_init_ccmd_dcamera (jmp) /
// 0x4db1d0 init_ccmd_dcamera. Confidence: exact.
console_command ccmd_dcamera("dcamera", "Change the dialog camera options.", cmd_dcamera);   // 0x2546750

// 0x4db1f0  Confidence: exact (an out-of-range value sets doing_help_for_comand, and the compiler jumps
// straight to the print).
void cmd_dcamera()
{
    if (calling_a_command_function) {
        process_command(ARG_INT);
        if (command_argument_int >= 0 && command_argument_int <= 2)
            Summoner_options.dialogue_camera = command_argument_int;
        else
            doing_help_for_comand = 1;
    }
    if (doing_help_for_comand)
        print_to_console("Usage: 0 -- always zoom, 1 -- never zoom, 2 -- zoom only on important characters", 0);
}
