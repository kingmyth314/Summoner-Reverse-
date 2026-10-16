// character_instance.cpp - Sum.exe v1.40, code 0x5194b0-0x51b6a1 (vsdk\ca\character_instance.cpp), plus
// the header inline functions this object emitted out of line (0x51b6b0-0x51b75f; see NOTES.md).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// The unit animates a ca_character: every anim (skeleton) has a time and a weight. State anims (walk,
// run, idle; skeleton::is_state) share one phase so they stay in step; action anims (attacks, hits) run
// on their own clock and fade in and out per bone (skeleton_get_bone_blend_weight). The bone matrices
// are blended from the active anims' key tracks and cached per frame; the root bone's motion is turned
// into a per-frame movement delta (get_motion_delta) and turn (take_turn_delta).
//
// Time units: 160 ticks per frame, 30 frames per second (0x12c0 = 4800 ticks per second).
// Header inline functions are called, not expanded, in this object (see NOTES.md, "inline expansion").

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vsdk/ca/character_instance.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/profile.h"
#include "vsdk/bmpman/bmpman.h"

#define CHARACTER_INSTANCE_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\ca\\character_instance.cpp"
#define TICKS_TO_SECONDS(t) ((t) * (1.0f / CI_TICKS_PER_FRAME) * (1.0f / CI_FRAMES_PER_SEC))

// ---- .data (0x5a8484) ------------------------------------------------------------------------------
// Root bone handling in build_bone_matrices: true = the root bone is placed at pos + its animated
// position; false = its motion is tracked for get_motion_delta. GameInit sets it to false (0x43da4a).
// Label ours.
bool ci_root_to_position = true;      // 0x5a8484

// ---- .bss (0x2710ec0-0x272281f) ---------------------------------------------------------------------
// The console command is defined first: its object is first in .bss and its two literals are first in
// .data (0x5a8488 "Toggles High_lod", 0x5a849c "high_lod"), although its initialiser is the last
// function of the unit (initialiser table 0x588908). Confidence: likely.
// 0x51b5c0 init_ccmd_high_lod_thunk (jmp 0x51b5d0) /
// 0x51b5d0 init_ccmd_high_lod: compiler-generated (console_command::console_command, ret).
console_command ccmd_high_lod("high_lod", "Toggles High_lod", cmd_high_lod);   // 0x2710ec0

// 0x5194b0 thunk / 0x5194c0 and
// 0x5194e0 init_vertex_buf_2719b70 (thunk) / 0x5194f0: the dynamic initialisers (vector constructor
// iterator with the out-of-line vector3() constructor; no destructor, so no atexit). Initialiser table
// entries 0x588900 / 0x588904. Nothing else references the two buffers. Labels ours. Confidence: likely.
vector3 ci_vertex_buf_a[3000];        // 0x2710ed0
vector3 ci_vertex_buf_b[3000];        // 0x2719b70

bool High_lod;                        // 0x2722810
// 0x2722818: the "" passed to mem_report_line by report_memory (an all-zero literal in .bss).

// 0x519510  Confidence: exact (the argument is pushed by the destructor and never read).
void character_instance::free_buffers(int unused)
{
    if (times) {
        delete times;
        times = NULL;
    }
    if (weights) {
        delete weights;
        weights = NULL;
    }
    if (bone_cache) {
        delete bone_cache;
        bone_cache = NULL;
    }
    if (skin_matrices) {
        delete skin_matrices;
        skin_matrices = NULL;
    }
}

// 0x519570  Confidence: likely (memset of the new arrays; the time/weight arrays are always 100 long).
// Only for a character with anims. The times start at each skeleton's first tick.
void character_instance::alloc_buffers()
{
    int num_bones = character->mesh->skin->num_bones;
    int anims = character->num_anims;
    if (anims) {
        bone_cache = new ci_bone_matrix[num_bones];
        memset(bone_cache, 0, num_bones * sizeof(ci_bone_matrix));
        skin_matrices = new matrix43[num_bones];
        memset(skin_matrices, 0, num_bones * sizeof(matrix43));
        times = new int[CI_MAX_ANIMS];
        weights = new float[CI_MAX_ANIMS];
        memset(weights, 0, CI_MAX_ANIMS * sizeof(float));
        ca_character *ch = character;
        int n = ch->num_anims;
        if (n > 0) {
            int *t = times;
            for (int i = 0; i < n; i++)
                t[i] = ch->skeletons[i]->start;
        }
    }
}

// 0x5196b0  Confidence: exact.
// For the static ui_character_instance (init_character_instance_5bd820); everything zero.
character_instance::character_instance()
{
    memset(this, 0, sizeof(*this));
}

// 0x519700  Confidence: exact (store order of the fields set after alloc_buffers).
character_instance::character_instance(ca_character *ch)
{
    memset(this, 0, sizeof(*this));
    held_anim = -1;
    action_anim = -1;
    state_anim = -1;
    character = ch;
    alloc_buffers();
    memset(trigger_hit, 0, sizeof(trigger_hit));
    phase = 0.0f;
    _unkbc = 2.0f;
    _unkc4 = 0.8f;
    _unkc0 = -1;
    _unkb4 = 0xff;
    pos = vector3(0.0f, 0.0f, 0.0f);
    turn_from = 0.0f;
    turn_to = 0.0f;
    motion_cur = vector3(0.0f, 0.0f, 0.0f);
    motion_prev = motion_cur;
    root_offset = vector3(0.0f, 0.0f, 0.0f);
    motion_valid = false;
    tint[0] = 0xff;
    tint[1] = 0xff;
    tint[2] = 0xff;
    for (int i = 0; i < CI_NUM_ANIM_SPEEDS; i++) {
        speeds[i].anim = -1;
        speeds[i].scale = 1.0f;
    }
}

// 0x519840  Confidence: exact.
character_instance::~character_instance()
{
    if (character)
        free_buffers(1);
}

// 0x519850  Confidence: exact.
// clear_weights clears 100 bytes, i.e. the first 25 weights only (memset size in bytes).
void character_instance::reset_anims(bool clear_weights)
{
    if (!weights)
        alloc_buffers();
    if (clear_weights) {
        memset(weights, 0, CI_MAX_ANIMS);
        ca_character *ch = character;
        int n = ch->num_anims;
        if (n > 0) {
            int *t = times;
            for (int i = 0; i < n; i++)
                t[i] = ch->skeletons[i]->start;
        }
    }
}

// 0x5198a0  Confidence: exact.
void character_instance::rewind_anim(int anim)
{
    weights[anim] = 0.0f;
    times[anim] = character->skeletons[anim]->start;
}

// 0x5198d0  Confidence: exact (the compound float operands; the temporaries of the vector expressions).
// Blends the keys of count anims into out: one key is used as is, two are slerped by the second weight,
// more are folded in one at a time with weight w / (sum so far). A zero m[0][0] is replaced by 1e-6.
void character_instance::blend_bone_keys(matrix43 *out, quaternion *rots, vector3 *positions,
                                         float *weights, int count)
{
    quaternion result;
    quaternion q(0.0f, 0.0f, 0.0f, 1.0f);
    vector3 p(0.0f, 0.0f, 0.0f);
    float total = 0.0f;

    switch (count) {
    case 1:
        out->set(rots[0], positions[0]);
        break;
    case 2: {
        quaternion q2;
        quaternion_slerp(&q2, &rots[0], &rots[1], weights[1]);
        vector3 p2;
        p2 = positions[0] * weights[0] + positions[1] * weights[1];
        out->set(q2, p2);
        break;
    }
    default:
        for (int i = 0; i < count; i++) {
            float w = weights[i];
            if (w != 0.0f) {
                total += w;
                quaternion_slerp(&result, &q, &rots[i], w / total);
                p = p + positions[i] * w;
                q = result;
            }
        }
        out->set(result, p);
        break;
    }
    if (out->m[0][0] == 0.0f)
        out->m[0][0] = 0.000001f;
}

// 0x519ac0  Confidence: unsure (control flow and calls follow the binary; local layout and the float
// expressions are guesses; recursive, about 8.6 KB of locals).
// Model-space matrix of one bone, cached per frame: blends the bone's keys of the active anims
// (anims[0..count-1]) by weight; while an action anim plays, the other anims are faded by its per-bone
// blend weight. Bones past num_bones are the extra (tag) bones. The root bone also produces the root
// motion (see ci_root_to_position).
matrix43 character_instance::build_bone_matrices(int bone, int count, ubyte *anims)
{
    quaternion rots[CI_MAX_BLEND];
    vector3 positions[CI_MAX_BLEND];
    float blend[CI_MAX_BLEND];
    ubyte used[CI_MAX_BLEND];

    if (bone < 0)
        return matrix43(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f),
                        vector3(0.0f, 0.0f, 1.0f), vector3(0.0f, 0.0f, 0.0f));

    ca_character *ch = character;
    if (bone >= ch->mesh->skin->num_bones) {
        if (bone - ch->mesh->skin->num_bones >= ch->mesh->skin->num_ext)
            printf("CHAR INST : %s\n", ch->name);
        if (bone - character->mesh->skin->num_bones >= character->mesh->skin->num_ext)
            return matrix43(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f),
                            vector3(0.0f, 0.0f, 1.0f), vector3(0.0f, 0.0f, 0.0f));
        return character->mesh->skin->ext[bone - character->mesh->skin->num_bones].m *
               build_bone_matrices(character->mesh->skin->ext[bone - character->mesh->skin->num_bones].parent,
                                   count, anims);
    }

    ci_bone_matrix *cache = &bone_cache[bone];
    if (cache->frame == frame)
        return cache->m;

    bool action_fade;
    float total = 0.0f;
    if (action_anim > -1 && weights[action_anim] != 0.0f)
        action_fade = true;
    else
        action_fade = false;

    int n = 0;
    for (int i = 0; i < count; i++) {
        int a = anims[i];
        float f = 1.0f;
        if (action_fade) {
            skeleton *action = ch->skeletons[action_anim];
            if (action->root_rotation && character->mesh->skin->root_bone == bone) {
                if (a != action_anim)
                    f = 0.0f;
            } else {
                f = (10.0f - skeleton_get_bone_blend_weight(action, bone, times[action_anim])) * 0.1f;
            }
        }
        skeleton *s = character->skeletons[a];
        if (s->is_state)
            f *= s->bone_weights[bone];
        else if (a == held_anim)
            f = 10.0f;
        else
            f = skeleton_get_bone_blend_weight(ch->skeletons[a], bone, times[a]);
        f *= weights[a];
        total += f;
        if (f != 0.0f) {
            blend[n] = f;
            used[n] = (ubyte)a;
            n++;
        }
    }
    if (total <= 0.0f) {
        total = 1.0f;
        blend[action_anim] = 1.0f;
    }

    float inv = 1.0f / total;
    int j;
    for (j = 0; j < n; j++) {
        blend[j] *= inv;
        int a = used[j];
        rots[j] = ch->skeletons[a]->tracks[bone].get_rotation(times[a], 0);
        positions[j] = ch->skeletons[a]->tracks[bone].get_position(times[a]);
    }

    matrix43 m;
    blend_bone_keys(&m, rots, positions, blend, n);
    int parent = character->mesh->skin->bones[bone].parent;
    if (parent > -1) {
        matrix43 parent_m = build_bone_matrices(parent, count, anims);
        bone_cache[bone].m = m * parent_m;
    } else {
        vector3 root;
        m.get_position(&root);
        if (ci_root_to_position) {
            m.set_position(pos + root);
        } else {
            if (action_anim > -1 && ch->skeletons[action_anim]->root_rotation) {
                matrix rot;
                float p, b, h;
                m.get_rotation(&rot);
                rot.extract_angles(&p, &b, &h);
                if (*(unsigned int *)&turn_start == CI_ANGLE_UNSET)
                    turn_start = 0.0f;
                turn_end = h;
            }
            motion_cur = motion_prev;
            motion_prev = root;
            motion_prev.y = 0.0f;
            if (!motion_valid) {
                motion_valid = true;
                motion_cur = motion_prev;
            }
            pos.y += root.y;
            m.set_position(pos);
        }
        pos = vector3(0.0f, 0.0f, 0.0f);
        bone_cache[bone].m = m;
    }
    bone_cache[bone].frame = frame;
    return bone_cache[bone].m;
}

// 0x51a130  Confidence: likely (min(f, w) written as the binary's "f > w" test).
// Weight 0..10 of an anim on one bone at a time: the bone's weight byte (10 for an action anim with a
// nonzero byte), ramped in over fade_in ticks from the start and out over fade_out ticks before the end.
float skeleton_get_bone_blend_weight(skeleton *s, int bone, int time)
{
    int w = s->bone_weights[bone];
    if (!w)
        return 0.0f;
    if (!s->is_state)
        w = 10;
    int t = time - s->start;
    int len = s->end - s->start;
    int fade_in = s->fade_in;
    int fade_out = s->fade_out;
    float f;
    if (t < fade_in)
        f = (float)t / fade_in * w;
    else if (t > len - fade_out)
        f = (float)(len - t) / fade_out * w;
    else
        f = (float)w;
    if (f > w)
        return (float)w;
    return f;
}

// 0x51a1e0  Confidence: exact.
void character_instance::advance_anim_time(int anim, int dt)
{
    float scale = 1.0f;
    for (int i = 0; i < CI_NUM_ANIM_SPEEDS; i++) {
        if (speeds[i].anim == anim) {
            scale = speeds[i].scale;
            break;
        }
    }
    times[anim] += (int)(dt * scale);
}

// 0x51a230  Confidence: unsure (three loops as in the binary; the two float sums live in x87 registers
// across the first loop, and the order of the operands of += is a guess).
// Advances all anims by dt ticks. The state anims share phase: it advances by the weighted mean of
// 1/length and every state anim's time is set from it; the heaviest state anim is trigger_anim and
// its triggers between the old and new time are latched in trigger_hit. Action anims run on their own
// clock (advance_anim_time); the one with the larger root blend weight becomes action_anim. An action
// anim that reaches its end stops (unless it is held_anim, which stays on the last tick) and frees its
// speed slot. Anim lengths under 1440 ticks count as 4800 for the phase rate.
void character_instance::increment_tick(int dt)
{
    int num = character->num_anims;
    profile_label label("character_instance::increment_tick");
    skeleton **skels = character->skeletons;
    ubyte anims[CI_MAX_ACTIVE];
    int lengths[CI_MAX_BLEND];
    float max_weight = 0.0f;
    float rate = 0.0f;
    float total = 0.0f;
    int i;

    trigger_anim = -1;
    int count = get_active_anims(this, anims);
    build_bone_matrices(character->mesh->skin->root_bone, count, anims);

    for (i = 0; i < num; i++) {
        skeleton *s = skels[i];
        lengths[i] = s->end - s->start;
        if ((float)lengths[i] < 1440.0f)
            lengths[i] = 4800;
        if (s->is_state) {
            rate += 1.0f / lengths[i] * weights[i];
            total += weights[i];
            if (weights[i] > max_weight) {
                trigger_anim = i;
                max_weight = weights[i];
            }
        }
    }

    if (total != 0.0f) {
        bool wrapped = false;
        phase += rate / total * dt;
        if (phase >= 1.0f) {
            wrapped = true;
            do
                phase -= 1.0f;
            while (phase >= 1.0f);
        }
        for (i = 0; i < num; i++) {
            skeleton *s = skels[i];
            if (s->is_state) {
                int old_time = times[i];
                if (weights[i] != 0.0f)
                    times[i] = (int)floor((s->end - s->start) * phase) + s->start;
                if (i == trigger_anim) {
                    for (int k = 0; k < CI_NUM_TRIGGERS; k++) {
                        int t = s->triggers[k].time;
                        if ((times[i] >= t && t > old_time) ||
                            (wrapped && (t > old_time || t < times[i])))
                            trigger_hit[k] = true;
                    }
                }
            } else if (weights[i] != 0.0f) {
                advance_anim_time(i, dt);
                if (action_anim == -1) {
                    turn_from = 0.0f;
                    turn_to = 0.0f;
                    action_anim = i;
                } else if (skeleton_get_bone_blend_weight(skels[i], character->mesh->skin->root_bone, times[i]) >
                           skeleton_get_bone_blend_weight(skels[action_anim], character->mesh->skin->root_bone,
                                                          times[action_anim])) {
                    turn_from = 0.0f;
                    turn_to = 0.0f;
                    action_anim = i;
                }
            }
        }
    } else {
        phase = 0.0f;
        for (i = 0; i < num; i++) {
            if (!skels[i]->is_state && weights[i] != 0.0f)
                advance_anim_time(i, dt);
        }
    }

    for (i = 0; i < num; i++) {
        skeleton *s = skels[i];
        if (!s->is_state && times[i] >= s->end) {
            if (i == held_anim) {
                times[i] = s->end;
            } else {
                if (i == action_anim)
                    action_anim = -1;
                if (weights[i] != 0.0f && character->skeletons[i]->root_rotation) {
                    turn_from = turn_start;
                    turn_to = turn_end;
                    *(unsigned int *)&turn_start = CI_ANGLE_UNSET;
                    *(unsigned int *)&turn_end = CI_ANGLE_UNSET;
                }
                weights[i] = 0.0f;
                times[i] = s->end;
                if (i == action_anim)
                    action_anim = -1;
            }
            for (int k = 0; k < CI_NUM_ANIM_SPEEDS; k++) {
                if (speeds[k].anim == i) {
                    speeds[k].anim = -1;
                    speeds[k].scale = 1.0f;
                }
            }
        }
        if (times[i] > s->end)
            times[i] = s->end;
        if (times[i] < s->start)
            times[i] = s->start;
    }
}

// 0x51a6a0  Confidence: exact.
// Indices of the first 8 anims with a nonzero weight.
int character_instance::get_active_anims(character_instance *ci, ubyte *out)
{
    int n = 0;
    int count = ci->character->num_anims;
    float *w = ci->weights;
    for (int i = 0; i < count; w++, i++) {
        if (*w != 0.0f) {
            out[n] = (ubyte)i;
            n++;
            if (n == CI_MAX_ACTIVE)
                break;
        }
    }
    return n;
}

// 0x51a6f0  Confidence: exact.
// Fills the bone cache (the results are dropped).
void character_instance::setup_matrices()
{
    profile_label label("character_instance::setup_matrices");
    ubyte anims[CI_MAX_ACTIVE];
    int count = get_active_anims(this, anims);
    int num_bones = character->mesh->skin->num_bones;
    for (int i = 0; i < num_bones; i++)
        build_bone_matrices(i, count, anims);
}

// 0x51a780  Confidence: exact.
// Skinning matrices for rendering: bind * bone for every bone. Nothing when culled.
void character_instance::setup_matrices_b()
{
    profile_label label("character_instance::setup_matrices");
    if (culled)
        return;
    ubyte anims[CI_MAX_ACTIVE];
    int count = get_active_anims(this, anims);
    ca_skin *skin = character->mesh->skin;
    ca_bone *bones = skin->bones;
    int num_bones = skin->num_bones;
    for (int i = 0; i < num_bones; i++)
        skin_matrices[i] = bones[i].bind * build_bone_matrices(i, count, anims);
}

// 0x51a870  Confidence: exact (empty; called by process with the frame time).
void character_instance::ca_noop_1arg(float frametime)
{
}

// 0x51a880  Confidence: exact (empty; called from luminar_beam_hit).
void character_instance::ca_noop_2arg(int a, int b)
{
}

// 0x51a890  Confidence: exact (the culled store is a compare and two stores in the binary).
// One frame: LOD (0 with the high_lod console command), frame counter 0..100, anims advanced by the
// frame time in ticks (frametime * 30 * 160).
void character_instance::process(float frametime, bool is_culled, int new_lod)
{
    profile_label label("character_instance::process");
    lod = new_lod;
    if (High_lod == true)
        lod = 0;
    if (++frame > 100)
        frame = 0;
    float frames = frametime * CI_FRAMES_PER_SEC;
    increment_tick((int)(frames * CI_TICKS_PER_FRAME));
    if (!is_culled)
        culled = false;
    else
        culled = true;
    ca_noop_1arg(frametime);
}

// 0x51a950  Confidence: likely (two different 0.1f constants: 0x5819a8 here, 0x580214 in
// build_bone_matrices; see NOTES).
// Weighted root velocity of the anims; the held anim counts only until its end. While an action anim
// plays, the state anims are faded by its root blend weight.
const vector3 &character_instance::get_root_velocity()
{
    root_velocity = vector3(0.0f, 0.0f, 0.0f);
    float total = 0.0f;
    float state_scale = 1.0f;
    if (action_anim > -1 && weights[action_anim] != 0.0f)
        state_scale = (10.0f - skeleton_get_bone_blend_weight(character->skeletons[action_anim],
                                                               character->mesh->skin->root_bone,
                                                               times[action_anim])) * 0.1f;
    for (int i = 0; i < character->num_anims; i++) {
        vector3 v = character->skeletons[i]->get_root_velocity();
        if (i != held_anim || times[i] < character->skeletons[i]->end) {
            float w;
            if (character->skeletons[i]->is_state)
                w = state_scale * weights[i];
            else
                w = skeleton_get_bone_blend_weight(character->skeletons[i], character->mesh->skin->root_bone,
                                                   times[i]) * weights[i];
            total += w;
            root_velocity += v * w;
        }
    }
    if (total != 0.0f)
        root_velocity *= 1.0f / total;
    return root_velocity;
}

// 0x51aab0  Confidence: exact.
// Movement of the root since the last frame plus the root velocity over dt; root_offset is the negated
// current root position. If the sum is not a valid vector it is computed again the same way.
vector3 character_instance::get_motion_delta(float dt)
{
    vector3 delta;
    if (!motion_valid) {
        delta = vector3(0.0f, 0.0f, 0.0f);
    } else {
        delta = motion_prev - motion_cur;
        root_offset = -motion_prev;
        delta += get_root_velocity() * dt;
        if (!delta.is_valid())
            delta = (motion_prev - motion_cur) + get_root_velocity() * dt;
    }
    return delta;
}

// 0x51abb0  Confidence: exact.
vector3 character_instance::get_root_offset(float unused)
{
    return root_offset;
}

// 0x51abd0  Confidence: likely.
// Weight-sum of the state anims' root velocities (the held anim only until its end). A NULL skeleton is a
// fatal error, line 1435.
vector3 character_instance::sum_root_velocity()
{
    root_velocity = vector3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < character->num_anims; i++) {
        skeleton *s = character->skeletons[i];
        if (!s) {
            sprintf(string_buffer, "Character has NULL anim: %s (%d)\n", character->name, i);
            VSDK_ERROR(CHARACTER_INSTANCE_CPP, 1435, string_buffer);
        }
        if (s->is_state) {
            vector3 v = s->get_root_velocity();
            if (i != held_anim || times[i] < character->skeletons[i]->end)
                root_velocity += v * weights[i];
        }
    }
    return root_velocity;
}

// 0x51acb0  Confidence: exact (the matrix local is constructed and not used).
// The turn accumulated since the last call, as a quaternion (0, 0, 0, delta), then cleared.
quaternion character_instance::take_turn_delta(float unused)
{
    matrix m;
    float d = turn_to - turn_from;
    turn_to = 0.0f;
    turn_from = 0.0f;
    quaternion q(0.0f, 0.0f, 0.0f, d);
    return q;
}

// 0x51ad00  Confidence: exact.
// Position of a bone's tag point (bone matrix times the bone's tag offset) and its tag value; zero for a
// bone out of range.
void character_instance::get_tag_pos(vector3 *out, int *value, int bone)
{
    if (bone >= character->mesh->skin->num_bones) {
        *out = vector3(0.0f, 0.0f, 0.0f);
        *value = 0;
        return;
    }
    matrix43 m = bone_cache[bone].m;
    *out = m.transform(character->mesh->skin->bones[bone].tag_offset);
    *value = character->mesh->skin->bones[bone].tag_value;
}

// 0x51adc0  Confidence: exact.
void character_instance::get_bounding_sphere(vector3 *center, float *radius)
{
    *center = character->mesh->vim->center;
    *radius = character->mesh->vim->radius;
}

// 0x51ae00  Confidence: exact (no caller found; the inverse is computed into a temporary and dropped).
void character_instance::get_bone_bind_inverse(int unused1, int unused2, vector3 *out_pos, matrix43 *m,
                                               int bone)
{
    *out_pos = character->mesh->skin->bones[bone].base_pos;
    *m = bone_cache[bone].m;
    m->inverse();
}

// 0x51ae70  Confidence: exact.
void ca_system_init_stub()
{
}

// 0x51ae80  Confidence: exact.
void ca_system_close_stub()
{
}

// 0x51ae90  Confidence: exact.
// Only for state anims: a positive weight makes the anim the dominant state.
void character_instance::set_state_weight(int anim, float weight)
{
    skeleton *s = character->skeletons[anim];
    if (s && s->is_state) {
        weights[anim] = weight;
        if (weight > 0.0f)
            state_anim = anim;
        else if (state_anim == anim)
            state_anim = -1;
    }
}

// 0x51aee0  Confidence: likely.
// Starts an anim from its first tick. A speed other than 1 is written into every free speed slot (there
// is no break in the loop).
void character_instance::start_anim(int anim, float weight, bool held, float speed)
{
    stop_anim(anim);
    weights[anim] = weight;
    skeleton *s = character->skeletons[anim];
    times[anim] = s->start;
    if (held == true)
        held_anim = anim;
    if (action_anim == -1) {
        action_anim = anim;
        turn_from = 0.0f;
        turn_to = 0.0f;
    }
    if (s->root_rotation) {
        *(unsigned int *)&turn_start = CI_ANGLE_UNSET;
        *(unsigned int *)&turn_end = CI_ANGLE_UNSET;
    }
    if (speed != 1.0f) {
        for (int i = 0; i < CI_NUM_ANIM_SPEEDS; i++) {
            if (speeds[i].anim == -1) {
                speeds[i].anim = anim;
                speeds[i].scale = speed;
            }
        }
    }
}

// 0x51af70  Confidence: exact.
// Only while the anim still has time left: updates its slot or takes the first free one.
void character_instance::set_anim_speed(int anim, float scale)
{
    if (get_anim_time_left(anim) != 0.0f) {
        int free_slot = -1;
        for (int i = 0; i < CI_NUM_ANIM_SPEEDS; i++) {
            if (speeds[i].anim == anim) {
                speeds[i].scale = scale;
                return;
            }
            if (speeds[i].anim == -1 && free_slot == -1)
                free_slot = i;
        }
        if (free_slot != -1) {
            speeds[free_slot].anim = anim;
            speeds[free_slot].scale = scale;
        }
    }
}

// 0x51afe0  Confidence: exact.
// Reads or writes the state phase (the boss riders copy it from the mount).
void character_instance::get_set_time(float *value, bool set)
{
    if (set)
        phase = *value;
    else
        *value = phase;
}

// 0x51b000  Confidence: exact.
float character_instance::get_anim_time_left(int anim)
{
    skeleton *s = character->skeletons[anim];
    if (!s)
        return 0.0f;
    if (weights[anim] == 0.0f)
        return 0.0f;
    int left = s->end - times[anim];
    float t = imax(0, left) * (1.0f / CI_TICKS_PER_FRAME);
    return t * (1.0f / CI_FRAMES_PER_SEC);
}

// 0x51b070  Confidence: exact (the result is in frames: seconds times 30).
float character_instance::get_anim_elapsed(int anim)
{
    if (weights[anim] == 0.0f)
        return 0.0f;
    float t = (times[anim] - character->skeletons[anim]->start) * (1.0f / CI_TICKS_PER_FRAME);
    float secs = t * (1.0f / CI_FRAMES_PER_SEC);
    return secs * 30.0f;
}

// 0x51b0c0  Confidence: exact.
float character_instance::get_anim_length(int anim)
{
    skeleton *s = character->skeletons[anim];
    int len = s->end - s->start;
    float t = imax(0, len) * (1.0f / CI_TICKS_PER_FRAME);
    return t * (1.0f / CI_FRAMES_PER_SEC);
}

// 0x51b100  Confidence: exact.
void character_instance::clear_state_anims()
{
    ca_character *ch = character;
    int n = ch->num_anims;
    for (int i = 0; i < n; i++) {
        if (ch->skeletons[i]->is_state == true)
            weights[i] = 0.0f;
    }
    state_anim = -1;
}

// 0x51b140  Confidence: exact (store order of the three resets).
void character_instance::clear_action_anims()
{
    ca_character *ch = character;
    int n = ch->num_anims;
    for (int i = 0; i < n; i++) {
        if (!ch->skeletons[i]->is_state)
            weights[i] = 0.0f;
    }
    action_anim = -1;
    held_anim = -1;
    motion_valid = false;
    motion_cur = vector3(0.0f, 0.0f, 0.0f);
    motion_prev = motion_cur;
}

// 0x51b1c0  Confidence: exact.
void character_instance::stop_anim(int anim)
{
    weights[anim] = 0.0f;
    if (anim == action_anim)
        action_anim = -1;
}

// 0x51b1e0  Confidence: exact (the zero is overwritten at once: there is no else).
void character_instance::get_anim_root_velocity(int anim, vector3 *out)
{
    if (!character->skeletons[anim]->is_state)
        *out = vector3(0.0f, 0.0f, 0.0f);
    *out = character->skeletons[anim]->get_root_velocity();
}

// 0x51b250  Confidence: exact.
float character_instance::get_anim_root_speed(int anim)
{
    if (!character->skeletons[anim]->is_state)
        return 0.0f;
    return character->skeletons[anim]->get_root_velocity().len();
}

// 0x51b290  Confidence: exact (strcmp is the inline intrinsic).
// True once per latched trigger of trigger_anim with that name (the latch is cleared).
bool character_instance::anim_trigger_hit(const char *name)
{
    int a = trigger_anim;
    if (a >= 0 && a < character->num_anims) {
        skeleton *s = character->skeletons[a];
        for (int i = 0; i < CI_NUM_TRIGGERS; i++) {
            if (s->triggers[i].name && !strcmp(name, s->triggers[i].name)) {
                if (trigger_hit[i]) {
                    trigger_hit[i] = false;
                    return true;
                }
                return false;
            }
        }
    }
    return false;
}

// 0x51b330  Confidence: exact.
void character_instance::page_in_textures()
{
    for (int i = 0; i < character->mesh->vim->num_textures; i++) {
        gr_page_in(character->mesh->vim->textures[i], 0);
        progress_update(0);
    }
}

// 0x51b380  Confidence: exact (ret 0x30 and no reference: an empty member taking 48 bytes of arguments,
// written as a matrix43 by value).
void character_instance::ca_stub_ret30_51b380(matrix43 m)
{
}

// 0x51b390  Confidence: exact (sum.symbols set_pos, formerly get_pos: it assigns pos = *v; no caller).
void character_instance::set_pos(const vector3 *v)
{
    pos = *v;
}

// 0x51b3b0  Confidence: exact.
matrix43 character_instance::compute_bone_matrix(int bone)
{
    ubyte anims[CI_MAX_ACTIVE];
    int count = get_active_anims(this, anims);
    return build_bone_matrices(bone, count, anims);
}

// 0x51b3f0  Confidence: exact.
// World transform of a bone for an instance placed at inst_pos with orientation inst_orient.
void character_instance::get_bone_world_transform(int bone, matrix *inst_orient, vector3 *inst_pos,
                                                  matrix *out_orient, vector3 *out_pos)
{
    matrix43 m = compute_bone_matrix(bone);
    m.get_rotation(out_orient);
    m.get_position(out_pos);
    *out_orient = *inst_orient * *out_orient;
    out_pos->unrotate(*inst_orient);
    *out_pos += *inst_pos;
}

// 0x51b460  Confidence: exact. Parameter order (out_pos, out_orient, bone): the bone is read from
// [esp+0xc], the rotation output from [esp+8], the position from [esp+4] (DECISIONS.md D302).
void character_instance::get_bone_transform(vector3 *out_pos, matrix *out_orient, int bone)
{
    matrix43 m = compute_bone_matrix(bone);
    m.get_rotation(out_orient);
    m.get_position(out_pos);
}

// 0x51b4a0  Confidence: exact (the three verbose lines never print: mem_report_verbose is always 0).
// Memory report: times and weights (4 bytes per anim each) and the bone cache (0x34 per bone). The
// label "cur tiume" is spelled so in the binary.
int character_instance::report_memory()
{
    mem_report_indent();
    mem_report_line(character->name, "");
    mem_report_indent();
    ca_character *ch = character;
    int num_anims = ch->num_anims;
    int num_bones = ch->mesh->skin->num_bones;
    int time_size = num_anims * sizeof(int);
    int pct_size = num_anims * sizeof(float);
    if (mem_report_is_verbose())
        mem_report_line("cur tiume :", "%d", time_size);
    if (mem_report_is_verbose())
        mem_report_line("anim pct :", "%d", pct_size);
    int bone_size = num_bones * sizeof(ci_bone_matrix);
    if (mem_report_is_verbose())
        mem_report_line("bone info :", "%d", bone_size);
    mem_report_unindent();
    int total = bone_size + time_size + pct_size;
    mem_report_line("TOTAL :", "%d", total);
    mem_report_unindent();
    return total;
}

// 0x51b560  Confidence: exact (the last call is a tail jump).
void ca_system_init(int arg)
{
    ca_system_init_stub();
    ca_init(arg);
    ca_init_globals_2ee4598();
}

// 0x51b580  Confidence: exact.
void ca_system_close()
{
    ca_system_close_stub();
    ca_close();
    ca_close_globals_544a40();
}

// 0x51b590  Confidence: exact.
void character_instance::set_tint(ubyte r, ubyte g, ubyte b)
{
    tint[0] = r;
    tint[1] = g;
    tint[2] = b;
}

// 0x51b5f0  Confidence: exact. Expansion of the console's bool command macro (as player.cpp).
void cmd_high_lod()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            High_lod = true;
        else if (arg_type & ARG_FALSE)
            High_lod = false;
        else if (arg_type & ARG_NONE)
            High_lod ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "high_lod", "High_lod");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "high_lod", High_lod ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// Header inline functions emitted out of line by this object (callers above), bodies in the headers:
// 0x51b6b0 profile_label::profile_label,
// 0x51b6c0 matrix43__copy: matrix43::operator= (vsdk/math/matrix.h), rep movsd of 12 dwords, no result.
// Confidence: exact.
// 0x51b6e0 matrix43::get_position (matrix43__get_position_b),
// 0x51b700 vector3__negated: vector3::operator-() (vsdk/math/vector.h), (-x, -y, -z) into the hidden
// result. Confidence: exact.
// 0x51b730 quaternion copy constructor (quaternion__assign_b).

// 0x51b750  Confidence: exact. imax, the last of these copies; defined here out of line (not inline in
// vector.h) so that its callers keep their calls under /Ob1, as imin in bmpman.cpp (D5606).
int imax(int a, int b)
{
    return a > b ? a : b;
}
