// unnamed_cutscene_camera_0x493e50.cpp - Sum.exe v1.40, code 0x493e50-0x494696: the camera sequences (the
// abstract camera_sequence, the cutscene camera shot read from a .csc file and sampled from its key
// tracks, the blend sequence's two methods) and the shots' key pools.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs fn). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: four static initialisers (entries 265-268, 0x493e50-0x493f10) open the unit, the AI group
// unit ends just before (cmd_party_controller_check); cutscene_camera_tracks_clear (0x494680) is last,
// the snow unit's initialisers (entries 269-270) follow at 0x4946a0.
// Data: .bss 0x243e3d8-0x2444597 (the four pool counts, then the pools), .rdata the two vtables
// (0x581ae4 base, 0x581af0 shot) and the literals 2.54 (0x581afc, shared with skeleton_read), 16383,
// 1.6949153, 4800 (0x581b00-0x581b08). No .data.

#include <windows.h>
#include <mmsystem.h>
#include "engine/camera/camera_sequence.h"
#include "engine/camera/camera.h"
#include "vsdk/ca/skeleton.h"
#include "vsdk/vfile/file.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "summoner/unnamed_cutscene_0x434900.h"

#define CAM_TICKS_PER_SECOND 4800.0f   // anim_track time units

// ---- .bss ------------------------------------------------------------------------------------------
int cutscene_cam_keys8_count;                                 // 0x243e3d8
int cutscene_cam_keys40_count;                                // 0x243e3dc
int cutscene_cam_keys20_count;                                // 0x243e3e0
// 0x493e50 init_cutscene_camera_keys_243e3e8_thunk (jmp, static initialiser entry 265). Confidence: exact.
// 0x493e60 init_cutscene_camera_keys_243e3e8: the array constructor of the position keys (anim_pos_key's
//          inline constructor: time 0, position = identity_matrix43's). Confidence: exact.
anim_pos_key cutscene_cam_keys40[CUTSCENE_CAM_POS_KEYS];       // 0x243e3e8
// 0x493eb0 init_cutscene_camera_keys_2440af8_thunk (jmp, entry 266). Confidence: exact.
// 0x493ec0 init_cutscene_camera_keys_2440af8: the rotation keys (time 0). Confidence: exact.
anim_rot_key cutscene_cam_keys20[CUTSCENE_CAM_ROT_KEYS];       // 0x2440af8
// 0x493ee0 init_empty_493ef0_thunk (jmp, entry 267). Confidence: likely.
// 0x493ef0 init_empty_493ef0 (ret): vector3's empty
//          constructor. Confidence: likely (which empty initialiser is which pool).
vector3 cutscene_cam_keys12[2 * CUTSCENE_CAM_POS_KEYS];        // 0x2441e80
int cutscene_cam_keys12_count;                                // 0x24435f0
// 0x493f00 init_empty_493f10_thunk (jmp, entry 268). Confidence: likely.
// 0x493f10 init_empty_493f10 (ret): cquat's empty
//          constructor. Confidence: likely.
cquat cutscene_cam_keys8[2 * CUTSCENE_CAM_ROT_KEYS];           // 0x24435f8

// ---- camera_sequence --------------------------------------------------------------------------------
// 0x493f20  Confidence: exact.
camera_sequence::camera_sequence(int state)
{
    this->state = state;
}

// 0x493f40 camera_sequence__scalar_deleting_dtor (compiler-generated: dtor, operator delete when bit 0).
// Confidence: exact.

// 0x493f60  Confidence: exact.
camera_sequence::~camera_sequence()
{
}

// 0x493f70  Confidence: exact.
int camera_sequence::get_state()
{
    return state;
}

// ---- cutscene_camera_sequence -----------------------------------------------------------------------
// 0x493f80  Confidence: exact.
cutscene_camera_sequence::cutscene_camera_sequence() : camera_sequence(0)
{
}

// 0x493fa0 cutscene_camera_sequence__scalar_deleting_dtor (compiler-generated). Confidence: exact.

// 0x493fc0  Confidence: exact (the vtable store and the jump to the base destructor are generated).
cutscene_camera_sequence::~cutscene_camera_sequence()
{
}

// 0x493fd0  Confidence: likely (the key loops' index registers). (file, start of this shot in the
// cutscene): a shot whose own start is at least 0.4 s later is moved back to it, its key times shifted
// with it. The keys come from the four pools, never checked against their sizes.
void cutscene_camera_sequence::read_track(File *f, float shot_start)
{
    int base;
    int k;

    start = f->read_float(0, 0);
    end = f->read_float(0, 0);
    if (shot_start >= 0.0f) {
        float d = start - shot_start;
        if (d >= 0.4f) {
            start -= d;
            end -= d;
            base = (int)(d * CAM_TICKS_PER_SECOND);
        } else {
            base = 0;
        }
    } else {
        base = 0;
    }
    fov = f->read_float(0, 0) * 1.6949153f;

    track.num_pos_keys = (ushort)f->read_int(0, 0);
    if (track.num_pos_keys > 0) {
        track.pos_keys = &cutscene_cam_keys40[cutscene_cam_keys40_count];
        cutscene_cam_keys40_count += track.num_pos_keys;
        track.pos_aux_a = &cutscene_cam_keys12[cutscene_cam_keys12_count];
        cutscene_cam_keys12_count += track.num_pos_keys;
        track.pos_aux_b = &cutscene_cam_keys12[cutscene_cam_keys12_count];
        cutscene_cam_keys12_count += track.num_pos_keys;
    }
    for (k = 0; k < track.num_pos_keys; k++) {
        track.pos_keys[k].time = f->read_int(0, 0) - base;
        f->read_vector(&track.pos_keys[k].pos, 0, &file_zero_vector);
        f->read_vector(&track.pos_keys[k].in, 0, &file_zero_vector);
        f->read_vector(&track.pos_keys[k].out, 0, &file_zero_vector);
    }

    track.num_rot_keys = (ushort)f->read_int(0, 0);
    if (track.num_rot_keys > 0) {
        track.rot_keys = &cutscene_cam_keys20[cutscene_cam_keys20_count];
        cutscene_cam_keys20_count += track.num_rot_keys;
        track.rot_aux_a = &cutscene_cam_keys8[cutscene_cam_keys8_count];
        cutscene_cam_keys8_count += track.num_rot_keys;
        track.rot_aux_b = &cutscene_cam_keys8[cutscene_cam_keys8_count];
        cutscene_cam_keys8_count += track.num_rot_keys;
    }
    for (k = 0; k < track.num_rot_keys; k++) {
        quaternion q;
        track.rot_keys[k].time = f->read_int(0, 0) - base;
        f->read_quaternion(&q, 0, &file_identity_quaternion);
        track.rot_keys[k].q.x = (short)(q.x * 16383.0f);
        track.rot_keys[k].q.y = (short)(q.y * 16383.0f);
        track.rot_keys[k].q.z = (short)(q.z * 16383.0f);
        track.rot_keys[k].q.w = (short)(q.w * 16383.0f);
        track.rot_keys[k].tangent[0] = (signed char)(f->read_float(0, 0) * 127.0f);
        track.rot_keys[k].tangent[1] = (signed char)(f->read_float(0, 0) * 127.0f);
        track.rot_keys[k].tangent[2] = (signed char)(f->read_float(0, 0) * 127.0f);
        track.rot_keys[k].ease_in = (signed char)(f->read_float(0, 0) * 2.54f);
        track.rot_keys[k].ease_out = (signed char)(f->read_float(0, 0) * 2.54f);
    }
    track.init_tangents(0);
}

// 0x494310  Confidence: exact (VC6 8966 byte match: the bounds parenthesised give fld / fld st(1) / fcompp, D12301; the clamp comparisons: `<=` for the start, `>=` for the end, as the
// branch conditions give them). The cutscene clock has the cutscene file's signed FISUB (GAME BUG G3,
// docs/sum-exe/timing-bugs.md).
void cutscene_camera_sequence::sample()
{
#ifdef FIXED_BUILD
    // part of FIXED_BUILD G3 (the camera sampler reads the same start time)
    float t = (float)(unsigned int)(timeGetTime() - sequence_start_time) * 0.001f + cutscene_time_offset;
#else
    float t = ((float)timeGetTime() - sequence_start_time) * 0.001f + cutscene_time_offset;
#endif
    if (t <= (start))
        t = start;
    if (t >= (end))
        t = end;
    int time = (int)(t * CAM_TICKS_PER_SECOND);
    pos = track.get_position(time);
    quaternion q = track.get_rotation(time, 1);
    q.to_matrix_b(&orient);
}

// 0x4943d0  Confidence: exact. Vtable 0x581af0 slot 1.
vector3 *cutscene_camera_sequence::get_pos()
{
    return &pos;
}

// 0x4943e0  Confidence: exact. Slot 2.
matrix3 *cutscene_camera_sequence::get_orient()
{
    return &orient;
}

// ---- camera_blend_sequence (D3401) ------------------------------------------------------------------
// 0x4943f0  Confidence: exact.
float camera_blend_sequence::time_left()
{
    return blend_out + blend_in + hold - elapsed;
}

// 0x494400  Confidence: exact (D19700: the target angles' addresses through pointer locals put each
// camera angle below its target angle, the binary's slots; the weights reuse the argument slot; the three
// angle unwraps are written out, no helper is called). The weight w ramps 0 -> 1 over blend_in, stays 1
// for hold and goes back to 0 over blend_out; position and Euler angles are mixed between the camera's
// view and the target view.
void camera_blend_sequence::advance(float dt)
{
    float p0, b0, h0, p1, b1, h1;
    float t;

    elapsed += dt;
    float total = hold + blend_in + blend_out;
    if (elapsed > total)
        elapsed = total;
    if (elapsed < blend_in) {
        t = elapsed / blend_in;
    } else {
        float up = hold + blend_in;
        if (up > elapsed)
            t = 1.0f;
        else
            t = 1.0f - (elapsed - up) / blend_out;
    }
    float w = tcb_ease(t, 0.25f, 0.25f);
    float u = 1.0f - w;

    float *tp = &p1, *tb = &b1, *th = &h1;
    cam->view_orient.extract_angles(&p0, &b0, &h0);
    target_orient.extract_angles(tp, tb, th);
    if (p0 - p1 > 3.1415927f)
        p0 -= 6.2831855f;
    else if (p1 - p0 > 3.1415927f)
        p1 -= 6.2831855f;
    if (b0 - b1 > 3.1415927f)
        b0 -= 6.2831855f;
    else if (b1 - b0 > 3.1415927f)
        b1 -= 6.2831855f;
    if (h0 - h1 > 3.1415927f)
        h0 -= 6.2831855f;
    else if (h1 - h0 > 3.1415927f)
        h1 -= 6.2831855f;
    orient.set_from_angles(p0 * u + p1 * w, b0 * u + b1 * w, h0 * u + h1 * w);
    pos = target_pos * w + cam->eye_pos * u;
}

// 0x494680  Confidence: exact. cutscene::load_files, before the shots are read.
void cutscene_camera_tracks_clear()
{
    cutscene_cam_keys40_count = 0;
    cutscene_cam_keys20_count = 0;
    cutscene_cam_keys12_count = 0;
    cutscene_cam_keys8_count = 0;
}
