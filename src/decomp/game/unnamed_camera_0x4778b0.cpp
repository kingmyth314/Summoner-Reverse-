// unnamed_camera_0x4778b0.cpp - Sum.exe v1.40, code 0x4778b0-0x479b59: the game camera (console commands
// zoom_limit / geom_avoid / mlook_sensitivity, input, auto-turn, eye and orientation smoothing, occluders,
// automatic view, modes, the state stack and cutscene shots).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; Summoner 2 names are no evidence). Rename the file when the
// name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs, llvm-objdump). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 /Ob1. Confidence per function: exact (control flow, calls, constants and globals match), likely (same,
// but one detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range (decomp58, NOTES.md, D3800): it opens with the static initialiser thunk of ccmd_zoom_limit
// (static_initializers entry 229; entries 230 / 231 are geom_avoid and mlook_sensitivity) after the goal
// unit's last RET (0x4778ab), and ends with camera_set_sequence_transform (ret 0x479b59); spells.cpp starts
// with entry 232 at 0x479b60 (strings.tbl spells.cpp ids). The source map's spells.cpp tag for 0x479ad0
// is a stray (the function only touches camera fields).
// .data 0x59b7fc-0x59b986: mlook_sensitivity, camera_zoom_max, camera_eye_offset_speed, then the literals
// (zoom_limit's from 0x59b808 to mlook_sensitivity's "...is %.3f\n" ending 0x59b983); spells.cpp's .data
// starts at 0x59b988 (spell_group_skill). The bool-command texts are pooled (0x588c34-0x588c50, /Gf).
// .bss 0x22e49b8-0x22e4b0c, between ai.cpp's (ai_combat_style_names ends 0x22e49b7) and spells.cpp's
// (spell_missiles 0x22e4b18). game_camera itself (0x60a018) is gameplay.cpp's.
// .rdata: this unit's float constants are 0x581a28-0x581a5c (after the goal unit's 0x581a0c-0x581a24).

#include <stdio.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/camera/camera.h"
#include "engine/camera/camera_sequence.h"
#include "engine/rendering/levelrender.h"
#include "levelscripts/level_scripts.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"

#define CAM_PI      3.14159265f       // 0x57f028
#define CAM_TWO_PI  6.28318531f       // 0x580e28
#define DIK_LCONTROL 0x1d
#define DIK_RCONTROL 0x9d

// A vector with an empty destructor (name ours, D3805). camera_frame's two function-local statics
// register atexit destructors (0x478d10 / 0x478d00, both a bare RET) and the function opens an EH frame
// (handler 0x57c528) whose state becomes 0 for the local copy of the target point: the type has a
// destructor that expands to nothing. vector3 itself has none (its globals register no atexit).
class cam_vec : public vector3 {
public:
    cam_vec(float x, float y, float z) : vector3(x, y, z) {}
    cam_vec(const vector3 &v) : vector3(v) {}
    ~cam_vec() {}
};

// ---- .data (0x59b7fc) -------------------------------------------------------------------------------
float mlook_sensitivity = 1.0f;       // 0x59b7fc
float camera_zoom_max = 1.0f;         // 0x59b800
float camera_eye_offset_speed = 1.5f; // 0x59b804 (name ours)

// ---- .bss (0x22e49b8-0x22e4b0c; definition order guessed) -------------------------------------------
int camera_occluders[MAX_CAMERA_OCCLUDERS];   // 0x22e49b8
// 0x22e4a38: camera_frame's static-local guard byte (compiler's). 0x22e4a3c-0x22e4abf: 0x84 bytes no
// instruction references (name ours; kept for the layout, D3805).
ubyte camera_unk_22e4a3c[0x84];       // 0x22e4a3c
int camera_occluders_unk;             // 0x22e4acc (name ours)
int camera_num_occluders;             // 0x22e4aec
bool Camera_geometry_avoidance;       // 0x22e4b0c

// 0x4778b0 init_ccmd_zoom_limit_thunk (jmp; static initialiser entry 229). Confidence: exact.
// 0x4778c0 init_ccmd_zoom_limit (console_command::console_command(0x22e4af0, ...)). Confidence: exact.
console_command ccmd_zoom_limit("zoom_limit", "Toggle camera zoom limiting.", cmd_zoom_limit);   // 0x22e4af0

// 0x4778e0  Confidence: exact (VC6 byte match) (the 1.0 store comes before the test). Not a toggle: "on" gives 1.0,
// anything else 1000.0. The texts have no newline.
void cmd_zoom_limit()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE);
        camera_zoom_max = 1.0f;
        if (!(arg_type & ARG_TRUE))
            camera_zoom_max = 1000.0f;
    }
    if (checking_status_for_command)
    {
        if (camera_zoom_max == 1.0f)
            print_to_console("Camera zoom limit is on.", 0);
        else
            print_to_console("Camera zoom limit is off.", 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: zoom_limit [on | off]", 0);
}

// 0x477960 init_ccmd_geom_avoid_thunk (jmp; entry 230). Confidence: exact.
// 0x477970 init_ccmd_geom_avoid (console_command constructor). Confidence: exact.
console_command ccmd_geom_avoid("geom_avoid", "Toggles Camera_geometry_avoidance", cmd_geom_avoid);   // 0x22e4b00

// 0x477990  Confidence: exact (the console's bool command macro; nothing reads the variable).
void cmd_geom_avoid()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Camera_geometry_avoidance = true;
        else if (arg_type & ARG_FALSE)
            Camera_geometry_avoidance = false;
        else if (arg_type & ARG_NONE)
            Camera_geometry_avoidance ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "geom_avoid", "Camera_geometry_avoidance");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "geom_avoid", Camera_geometry_avoidance ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x477a50 init_ccmd_mlook_sensitivity_thunk (jmp; entry 231). Confidence: exact.
// 0x477a60 init_ccmd_mlook_sensitivity (console_command constructor). Confidence: exact.
console_command ccmd_mlook_sensitivity("mlook_sensitivity", "Sets the sensitivity value for mouse look.",
                                       cmd_mlook_sensitivity);   // 0x22e4ae0

// 0x477a80  Confidence: exact.
void cmd_mlook_sensitivity()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        mlook_sensitivity = command_argument_float;
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Mouse look sensitivity is %.3f\n", mlook_sensitivity);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Sets the mouse look sensitivity.  Higher values cause higher sensitivity.\n", 0);
}

// ---- the camera -------------------------------------------------------------------------------------
// D8902: float clamps through by-value inline functions (D7500): the value stays on the FPU (`fcom`, then
// `fstp st(0); fld c` when it is clamped), which the vsdk.h min / max macros do not give. Names ours.
static inline float cam_max(float a, float b) { return a > b ? a : b; }
static inline float cam_min(float a, float b) { return a < b ? a : b; }

// 0x477af0  Confidence: exact (VC6 byte match, match30; was likely; D8902: by-value inline float min / max). Earlier note: (both branches of the zoom floor clamp to 0.3: the code tests the
// elevation and then does the same thing twice). A full turn takes 6 s in the low view, 7.5 s in the high
// view. On the world map the elevation is fixed at 12 and the zoom at 1.
void camera::apply_input_deltas()
{
    float turn_time = 6.0f;
    if (!(elevation < 4.0f))
        turn_time = 7.5f;
    yaw += reported_frametime * CAM_TWO_PI / turn_time * input_yaw;
    elevation += reported_frametime * input_elevation * 4.0f;
    if (!(flags & CAMF_ZOOM_BLOCKED))
        zoom += reported_frametime * input_zoom;
    if (yaw > CAM_TWO_PI)
        yaw -= CAM_TWO_PI;
    else if (yaw < -CAM_TWO_PI)
        yaw += CAM_TWO_PI;
    if (elevation < 4.0f)
        zoom = cam_max(zoom, 0.3f);
    else
        zoom = cam_max(zoom, 0.3f);
    zoom = cam_min(zoom, camera_zoom_max);
    if (cur_level_index == 4) {
        elevation = cam_max(elevation, 12.0f);
        elevation = cam_min(elevation, 12.0f);
        zoom = 1.0f;
        return;
    }
    elevation = cam_max(elevation, -2.0f);
    elevation = cam_min(elevation, 9.0f);
}

// 0x477ca0  Confidence: exact (VC6 byte match) (the two quadrant fix-ups are the code's, odd as they are). Turns the yaw
// toward wanted_dir by at most 45 degrees per second of difference; a low, close view first zooms out
// (flag 0x40 blocks the zoom input, 0x100 restores the zoom afterwards).
void camera::auto_rotate_update()
{
    uint was_blocked = (flags >> 6) & 1;
    float a, d;

    flags &= ~CAMF_ZOOM_BLOCKED;
    if (wanted_dir.is_zero()) {
        if (flags & CAMF_RESTORE_ZOOM) {
            zoom = saved_zoom;
            flags &= ~CAMF_RESTORE_ZOOM;
        }
        return;
    }
    if (wanted_dir.z != 0.0f)
        a = (float)atan(wanted_dir.x / wanted_dir.z);
    else
        a = 0.0f;
    if (wanted_dir.x < 0.0f && wanted_dir.z > 0.0f)
        a += CAM_PI;
    if (wanted_dir.x > 0.0f && wanted_dir.z > 0.0f)
        a -= CAM_PI;
    while (yaw - a >= CAM_PI)
        a += CAM_TWO_PI;
    while (yaw - a < -CAM_PI)
        a -= CAM_TWO_PI;
    d = yaw - a;
    if (fabsf(d) > 0.17453294f) {
        if (elevation < 4.0f && zoom < 0.8f) {
            if (!was_blocked) {
                saved_zoom = zoom;
                flags |= CAMF_RESTORE_ZOOM;
            }
            flags |= CAMF_ZOOM_BLOCKED;
            zoom += reported_frametime;
            return;
        }
        if (d > 0.0f)
            d = min(d, CAM_PI / 4);
        else
            d = max(d, -CAM_PI / 4);
        yaw -= reported_frametime * d;
    }
    wanted_dir.set(0.0f, 0.0f, 0.0f);
}

// 0x477ea0  Confidence: exact (VC6 byte match, cl 8966, match21; was likely; x, z, y are computed in that order). ret 4 (hidden result).
vector3 camera::compute_eye_pos()
{
    vector3 v;
    v.x = (float)sin(yaw) * distance * zoom + look_at.x;
    v.z = (float)cos(yaw) * distance * zoom + look_at.z;
    v.y = elevation * zoom + height[0] + eye_offset;
    return v;
}

// 0x477f10  Confidence: exact (VC6 byte match, cl 8966, match21; was likely; as compute_eye_pos). ret 4 (hidden result).
vector3 camera::get_target_offset()
{
    vector3 v;
    v.x = look_at.x - eye_pos.x;
    v.z = look_at.z - eye_pos.z;
    v.y = height[0] - eye_pos.y + eye_offset;
    return v;
}

// 0x477f60  Confidence: likely (the three ring walks). Outside orient mode 2 the view looks straight at
// the target and the history restarts; in mode 2 the last four offsets are averaged, weighted by their
// frame times.
void camera::update_orientation()
{
    vector3 avg;
    float w[4];
    float total;
    int i, n;
    vector3 off = get_target_offset();

    if (orient_mode != 2) {
        dir_history[0] = off;
        dt_history[0] = reported_frametime;
        history_index = 1;
        history_count = 1;
        eye_orient.set_from_forward(off);
        return;
    }
    dir_history[history_index] = off;
    dt_history[history_index] = reported_frametime;
    history_index = (history_index + 1) % 4;
    if (history_count < 4)
        history_count++;
    total = 0.0f;
    i = history_index;
    for (n = history_count; n > 0; n--) {
        if (--i < 0)
            i = 3;
        total += dt_history[i];
    }
    i = history_index;
    for (n = history_count; n > 0; n--) {
        if (--i < 0)
            i = 3;
        w[i] = dt_history[i] / total;
    }
    avg.zero();
    n = history_count;
    i = history_index;
    for (int k = n; k > 0; k--) {
        if (--i < 0)
            i = 3;
        avg += dir_history[i] * w[i];
    }
    avg *= 1.0f / n;
    eye_orient.set_from_forward(avg);
}

// 0x478130  Confidence: likely (the acceleration expression; the length test's term order). The eye
// moves toward the wanted eye with the acceleration that reaches it in k seconds (k 0.5, 0.3 in mode 5,
// less for a steep high view). A moving living target sets orient mode 2; a settled eye mode 0, and
// turning input mode 1.
void camera::update_eye_position()
{
    vector3 wanted;
    float k, r;

    if (mode == 5)
        wanted = fixed_eye;
    else
        wanted = compute_eye_pos();
    vector3 d = wanted - eye_pos;
    k = (mode == 5) ? 0.3f : 0.5f;
    if (elevation >= 4.0f) {
        r = ((float)sqrt(d.x * d.x + d.z * d.z) + distance * zoom) / (zoom * elevation + d.y);
        if (r > 1.3f)
            k = 0.25f;
        else if (r >= 0.857f)
            k = k - (r - 0.857f) * (k - 0.25f) * 2.25734f;
    }
    vector3 accel = (d - eye_velocity * k) * 2.0f * (1.0f / (k * k));
    camera_approach_value(&eye_pos.x, &eye_velocity.x, accel.x, reported_frametime, wanted.x);
    camera_approach_value(&eye_pos.y, &eye_velocity.y, accel.y, reported_frametime, wanted.y);
    camera_approach_value(&eye_pos.z, &eye_velocity.z, accel.z, reported_frametime, wanted.z);
    if (eye_offset_wanted - eye_offset >= 0.0f) {
        eye_offset += camera_eye_offset_speed * reported_frametime;
        if (eye_offset > eye_offset_wanted)
            eye_offset = eye_offset_wanted;
    } else {
        eye_offset -= camera_eye_offset_speed * reported_frametime;
        if (eye_offset < eye_offset_wanted)
            eye_offset = eye_offset_wanted;
    }
    if (target) {
        if (target->is_living() && ((Character *)target)->forward_speed > 0.01f)
            orient_mode = 2;
        else if ((eye_pos - wanted).len() < 0.001f)
            orient_mode = 0;
        if (orient_mode == 0 && fabs(input_yaw) > 0.0f)
            orient_mode = 1;
    }
}

// 0x478450  Confidence: exact. cdecl. The sign of the acceleration is the caller's; the step snaps to
// the target when the remaining distance changes sign.
void camera_approach_value(float *value, float *velocity, float accel, float dt, float target)
{
    float before = (target - *value < 0.0f) ? -1.0f : 1.0f;
    float dv = accel * dt;
    float after;

    *value += (dv * 0.5f + *velocity) * dt;
    *velocity += dv;
    after = (target - *value < 0.0f) ? -1.0f : 1.0f;
    if (before != after) {
        *value = target;
        *velocity = 0.0f;
    }
}

// 0x4784e0  Confidence: exact (VC6 byte match) (thiscall with four arguments, ret 0x10; only the index is read).
void camera::add_occluder(vector3 *a, int lobj_index, int start_count, float radius)
{
    int i;

    if (camera_num_occluders >= MAX_CAMERA_OCCLUDERS)
        return;
    for (i = 0; i < camera_num_occluders; i++) {
        if (lobj_index == camera_occluders[i])
            return;
    }
    camera_occluders[camera_num_occluders++] = lobj_index;
}

// 0x478520  Confidence: likely (the ray's constructor store comes after the cutscene test, so the ray is
// declared there; the early return is `xor al, al`). Two rays: b -> a, then a -> b, each repeated
// through the draw lists until nothing more is hit. ret 0xc.
int camera::find_occluders(vector3 *a, vector3 *b, float radius)
{
    int start_count = camera_num_occluders;
    int index = 0;
    bool blocked = false;
    lobj_hit hit;

    if (game_flags & GF_CUTSCENE)
        return 0;
    lobj_ray ray;
    ray.start = *b;
    ray.dir = *a - *b;
    ray._unk18 = radius;
    ray.flags = 0;
    ray.require_flags = 2;
    ray.exclude_flags = 0;
    do {
        lobj_raycast_draw_lists(&ray, &hit, &index, &blocked, 1);
        if (hit.num_hits > 0)
            add_occluder(a, hit.face, start_count, radius);
    } while (hit.num_hits && camera_num_occluders < MAX_CAMERA_OCCLUDERS);
    if (camera_num_occluders < MAX_CAMERA_OCCLUDERS) {
        ray.start = *a;
        ray.dir = *b - *a;
        index = 0;
        blocked = false;
        do {
            lobj_raycast_draw_lists(&ray, &hit, &index, &blocked, 1);
            if (hit.num_hits > 0)
                add_occluder(a, hit.face, start_count, radius);
        } while (hit.num_hits && camera_num_occluders < MAX_CAMERA_OCCLUDERS);
    }
    if (camera_num_occluders > 0 || camera_occluders_unk > 0)
        return 1;
    return 0;
}

// 0x478700  Confidence: exact (VC6 byte match) (h's slot holds the difference). A new wanted height only past 0.3; the
// height moves at 4 x the difference per second and snaps within 0.001.
// GAME BUG (NOTES): when a step would overshoot (frame time above 0.25 s) it is replaced by -difference,
// which moves the height away from the wanted one.
void camera::smooth_height(float h)
{
    if (fabsf(height[0] - h) > 0.3f)
        height[1] = h;
    if (height[0] != height[1]) {
        float d = height[1] - height[0];
        float step = reported_frametime * d * 4.0f;
        if (fabsf(step) > fabsf(d))
            step = -d;
        height[0] += step;
        if (fabsf(height[1] - height[0]) < 0.001f)
            height[0] = height[1];
    }
}

// 0x4787b0  Confidence: likely. Mode 2 only, once a second: high at once while an enemy is near, low
// again 5 s after the last one.
void camera::auto_view_update()
{
    bool near_enemy;

    if (mode != 2 || !target)
        return;
    if ((game_flags & GF_SOLO_MODE) && elevation < 4.0f)
        return;
    if (game_flags & GF_CONVERSATION)
        return;
    if (Ai_paused)
        return;
    if (!auto_view_timer.elapsed())
        return;
    auto_view_timer.SetTimeout(1000);
    if (!(flags & CAMF_FOLLOW_TARGET))
        return;
    near_enemy = enemy_near_leader();
    if (elevation < 4.0f) {
        if (near_enemy == true)
            view_high();
    } else if (!near_enemy) {
        if (enemy_near == true) {
            auto_view_timer.SetTimeout(5000);
            enemy_near = near_enemy;
            return;
        }
        view_low();
    }
    enemy_near = near_enemy;
}

// 0x478890  Confidence: exact. ret 4.
void camera::get_state(uint *out)
{
    out[0] = flags;
    ((float *)out)[1] = distance;
    ((float *)out)[2] = yaw;
    ((float *)out)[3] = zoom;
    ((float *)out)[4] = elevation;
    ((float *)out)[5] = fov;
    ((float *)out)[6] = look_height;
    out[7] = mode;
    out[8] = prev_mode;
}

// 0x4788e0  Confidence: exact. ret 4.
void camera::set_state(const uint *in)
{
    flags = in[0];
    distance = ((const float *)in)[1];
    yaw = ((const float *)in)[2];
    zoom = ((const float *)in)[3];
    elevation = ((const float *)in)[4];
    fov = ((const float *)in)[5];
    look_height = ((const float *)in)[6];
    mode = in[7];
    prev_mode = in[8];
}

// 0x478930  Confidence: exact (the fog call is a tail jump).
void camera::view_high()
{
    set_view_params(6.0f, 7.0f, 1.0f, 0.3f, 1.0f);
    fog_set_high_camera_default();
}

// 0x478960  Confidence: exact.
void camera::view_low()
{
    set_view_params(6.0f, 2.0f, 0.8f, 3.3f, -1.5f);
    fog_set_low_camera_default();
}

// 0x478990  Confidence: likely (the static guards and the EH state come from cam_vec's empty destructor;
// see the class). Reads the target's position without a NULL test.
// The two static destructors are compiler-generated (after the function, inside this comment block so
// that the function keeps its own address):
// 0x478d00 camera_frame_static_dtor_b (atexit destructor of last_target_pos, a bare RET). Confidence: unsure (D3805).
// 0x478d10 camera_frame_static_dtor_a (of last_pos, a bare RET; guard 0x22e4a38 bit 1 per sum.symbols).
//          Confidence: unsure (D3805).
void camera::frame()
{
    static cam_vec last_pos(0.0f, 0.0f, 0.0f);          // 0x22e4ac0 (guard 0x22e4a38 bit 0)
    static cam_vec last_target_pos(0.0f, 0.0f, 0.0f);   // 0x22e4ad0 (guard bit 1)

    if (target && (flags & CAMF_FOLLOW_TARGET)) {
        update_target_pos();
        apply_input_deltas();
        auto_rotate_update();
        smooth_height(look_at.y);
        update_eye_position();
        update_orientation();
    }
    if ((target->pos - last_target_pos).len_sq() > 0.0064f)
        last_target_pos = target->pos;
    if ((eye_pos - last_pos).len_sq() > 0.0064f)
        last_pos = eye_pos;
    cam_vec p = last_target_pos;
    camera_num_occluders = 0;
    camera_occluders_unk = 0;
    p.y = last_target_pos.y + min(look_height, target->bound_radius * 0.8f);
    find_occluders(&p, &last_pos, 0.0f);
    if (elevation < 4.0f)
        zoom_low = zoom;
    else
        zoom_high = zoom;
    input_yaw = 0.0f;
    input_zoom = 0.0f;
    input_elevation = 0.0f;
    auto_view_update();
    if (target && (flags & CAMF_FOLLOW_TARGET) && (flags & CAMF_FOLLOW_VIEW)) {
        copy_follow_view();
        return;
    }
    if (flags & CAMF_SEQUENCE) {
        update_sequence();
        if (sequence->get_state() == 1 && ((camera_blend_sequence *)sequence)->time_left() == 0.0f)
            end_sequence(true);
    } else if (mode == 5) {
        float speed;
        update_eye_position();
        copy_follow_view();
        speed = 0.0f;
        if (target->is_living())
            speed = ((Character *)target)->forward_speed;
        if (orient_mode == 0 || speed > 0.01f) {
            flags |= CAMF_FOLLOW_TARGET;
            set_mode(4);
        }
    }
    if (!(flags & CAMF_FOLLOW_VIEW) && (flags & CAMF_FREE_LOOK))
        free_look_update();
}

// 0x478d20  Confidence: exact (camera_resume_follow is a tail jump).
void camera_level_reset()
{
    camera_fov = 73.74f;
    camera_num_occluders = 0;
    camera_occluders_unk = 0;
    game_camera.resume_follow();
}

// 0x478d40  Confidence: exact.
void camera_leave_level()
{
    if (game_camera.mode == 5 || game_camera.mode == 3) {
        game_camera.resume_follow();
        game_camera.set_mode(4);
    }
}

// 0x478d70  Confidence: exact. The fov is 73.74 degrees.
camera::camera()
{
    high_view_suppressed = 0;
    high_view_requested = 0;
    sequence = NULL;
    target = NULL;
    num_saved_states = 0;
    yaw = 0.0f;
    zoom = 0.0f;
    elevation = 0.0f;
    fov = 73.74f;
    distance = 0.0f;
    look_height = 0.0f;
    history_index = 0;
    history_count = 0;
    set_follow_flags(false, false);
    zoom_low = 0.8f;
    zoom_high = 1.0f;
}

// 0x478df0  Confidence: exact.
camera::~camera()
{
    if (flags & CAMF_SEQUENCE)
        end_sequence(true);
}

// 0x478e00  Confidence: exact (VC6 byte match) (both branches end in one store of z).
void camera::update_target_pos()
{
    if (!target) {
        look_at.x = 0.0f;
        look_at.y = 0.0f;
        look_at.z = 0.0f;
    } else
        look_at = target->pos + target->centre_offset;
    look_at.y += look_height;
}

// 0x478e70  Confidence: exact. ret 8. full: the eye, its history and the orientation jump to the target.
void camera::snap_to_target(bool update_target, bool full)
{
    if (update_target == true)
        update_target_pos();
    set_height(look_at.y);
    eye_offset = eye_offset_wanted;
    if (full == true) {
        if (cur_level_index == 4) {
            elevation = 12.0f;
            zoom = 1.0f;
        }
        eye_pos = compute_eye_pos();
        history_index = 0;
        vector3 off = get_target_offset();
        history_count = 0;
        eye_velocity.zero();
        _unk230 = 0;
        orient_mode = 0;
        eye_orient.set_from_forward(off);
    }
    auto_view_timer.SetTimeout(0);
    enemy_near = 0;
    flags &= ~CAMF_RESTORE_ZOOM;
}

// 0x478f60  Confidence: exact.
void camera::reset_default()
{
    target = NULL;
    wanted_dir.set(0.0f, 0.0f, 0.0f);
    if (!(flags & CAMF_DEFAULTS_SET))
        set_default_view_params(6.0f, 2.0f, 0.8f, 3.3f, -1.5f);
    mode = -1;
    set_mode(2);
    prev_mode = 2;
    snap_to_target(true, true);
}

// 0x478fd0  Confidence: exact. ret 4.
void camera::set_height(float h)
{
    height[1] = height[0] = h;
}

// 0x478ff0  Confidence: exact. ret 4. A new target more than 10 units away snaps the camera; a closer one
// glides there in mode 5. Without the follow-view flag only the target changes.
void camera::set_target(Character *c)
{
    float d;

    if (c == target)
        return;
    d = 999999.0f;
    if (target)
        d = ::distance(&target->pos, &c->pos);
    target = c;
    if (!(flags & CAMF_FOLLOW_VIEW))
        return;
    if (d > 10.0f) {
        snap_to_target(true, true);
        if (flags & CAMF_FOLLOW_VIEW)
            copy_follow_view();
        return;
    }
    snap_to_target(true, false);
    fixed_eye = compute_eye_pos();
    flags &= ~CAMF_FOLLOW_TARGET;
    orient_mode = 2;
    set_mode(5);
}

// 0x4790d0  Confidence: exact.
void camera::copy_follow_view()
{
    if (flags & CAMF_FOLLOW_VIEW) {
        view_pos = eye_pos;
        view_orient = eye_orient;
    }
}

// 0x479170  Confidence: exact. ret 0xc.
void camera::set_input(float yaw_rate, float zoom_rate, float elevation_rate)
{
    input_yaw = yaw_rate;
    input_zoom = zoom_rate;
    input_elevation = elevation_rate;
}

// 0x4791a0  Confidence: exact (VC6 byte match) (the matrix assignment is expanded). With a Ctrl key held the mouse turns
// the rendered view; the keys then move it (movement_read_controls).
void camera::free_look_update()
{
    int dx, dy, dz;
    float dt;

    if ((flags & CAMF_FOLLOW_VIEW) || !(flags & CAMF_FREE_LOOK))
        return;
    dt = reported_frametime;
    if (key_is_down(DIK_LCONTROL) == true || key_is_down(DIK_RCONTROL) == true) {
        mouse_get_delta(&dx, &dy, &dz);
        if (dx || dy) {
            matrix rot(dy * mlook_sensitivity * 0.01f, 0.0f, dx * mlook_sensitivity * 0.01f);
            view_orient = view_orient * rot;
            view_orient.orthogonalize();
        }
    }
    movement_read_controls(&view_pos, &view_orient, dt);
}

// 0x4792c0  Confidence: exact (VC6 byte match) (the two negated rows; the shot axes are (right, -forward, -up); the fov
// before the flag, D9400). ret 4.
// Ends a running shot (with the pop), pushes the state, then takes the shot's first view; the shot's
// +0x08 times 1/pi is the fov.
void camera::start_sequence(cutscene_camera_sequence *seq)
{
    if (flags & CAMF_SEQUENCE)
        end_sequence(true);
    sequence = seq;
    if (!seq)
        return;
    seq->sample();
    push_state();
    set_follow_flags(false, false);
    matrix m(*seq->get_orient());
    vector3 up = -m.fvec;
    vector3 fwd = -m.uvec;
    view_orient.set_from_forward_up(fwd, up);
    view_pos = *seq->get_pos();
    if (flags & CAMF_SEQ_TRANSFORM) {
        view_pos.unrotate(seq_orient);
        view_pos += seq_origin;
        view_orient = seq_orient * view_orient;
    }
    fov = seq->fov * 0.31830987f;     // 1/pi, 0x3ea2f983 (D9400)
    flags |= CAMF_SEQUENCE;
}

// 0x479430  Confidence: exact. ret 0x14.
void camera::cut_to_view_stub(vector3 *pos, matrix3 *orient, float distance, float a, float b)
{
}

// 0x479440  Confidence: exact (VC6 byte match) (switch on the state; the delete tests the pointer again). A shot in state
// 1 (a blend, D3806) is deleted (scalar deleting destructor, flag 1); a shot (state 0) is only dropped.
void camera::free_sequence()
{
    if (!sequence)
        return;
    switch (sequence->get_state()) {
    case 1:
        delete sequence;
        /* fall through */
    case 0:
        sequence = NULL;
        break;
    }
}

// 0x479480  Confidence: exact (VC6 byte match) (the member reloaded for each use, no local, D9400; the
// row copies of the second case are expanded with the vector3 copy constructor 0x410cd0 and the
// assignment 0x413190 out of line, the first case's inline). Frozen while the game is paused. State 0: the shot's first view (as camera_start_sequence); state 1 (a blend,
// camera_blend_sequence, D3806): advance, then copy its view.
void camera::update_sequence()
{
    if (!sequence || !(flags & CAMF_SEQUENCE) || (game_flags & GF_PAUSED))
        return;
    switch (sequence->get_state()) {
    case 0: {
        ((cutscene_camera_sequence *)sequence)->sample();
        matrix m(*sequence->get_orient());
        vector3 up = -m.fvec;
        vector3 fwd = -m.uvec;
        view_orient.set_from_forward_up(fwd, up);
        view_pos = *sequence->get_pos();
        fov = ((cutscene_camera_sequence *)sequence)->fov * 0.31830987f;   // D9400
        if (flags & CAMF_SEQ_TRANSFORM) {
            view_pos.unrotate(seq_orient);
            view_pos += seq_origin;
            view_orient = seq_orient * view_orient;
        }
        break;
    }
    case 1:
        ((camera_blend_sequence *)sequence)->advance(reported_frametime);
        view_pos = *sequence->get_pos();
        view_orient = *sequence->get_orient();
        break;
    }
}

// 0x4796a0  Confidence: exact. thiscall, this unused. The leader's AI target when it is an enemy, or a
// living enemy that is not a party member within 8 units.
bool camera::enemy_near_leader()
{
    Character *c;

    if (!cur_character)
        return false;
    if (cur_character->ai.target != -1) {
        c = living_from_handle(cur_character->ai.target);
        if (c && cur_character->is_enemy(c))
            return true;
    }
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->type != 7 && cur_character->is_enemy(c) && distance_squared(&c->pos, &cur_character->pos) < 64.0f)
            return true;
    }
    return false;
}

// 0x479740  Confidence: exact (VC6 byte match) (three bit-field stores merged: flag 0x1 cleared, 0x2 = follow, 0x10 =
// free look). ret 8. A running shot ends first, popping the state unless free look is asked for.
void camera::set_follow_flags(bool a, bool b)
{
    if (flags & CAMF_SEQUENCE)
        end_sequence(!b);
    flags = (flags & ~(CAMF_FOLLOW_VIEW | CAMF_FOLLOW_TARGET | CAMF_FREE_LOOK))
          | (((((b != 0) & 1) << 3) | ((a != 0) & 1)) << 1);
}

// 0x479790  Confidence: exact.
void camera::resume_follow()
{
    if (flags & CAMF_SEQUENCE)
        end_sequence(true);
    flags |= CAMF_FOLLOW_VIEW | CAMF_FOLLOW_TARGET;
}

// 0x4797b0  Confidence: exact. At most 8 saved states; more are dropped.
void camera::push_state()
{
    if (num_saved_states < 8) {
        get_state(saved_states[num_saved_states]);
        num_saved_states++;
    }
}

// 0x4797d0  Confidence: exact.
void camera::pop_state()
{
    if (num_saved_states > 0) {
        num_saved_states--;
        set_state(saved_states[num_saved_states]);
    }
}

// 0x4797f0  Confidence: exact. ret 4. game_flags 0x800 is cleared with the sequence flag.
void camera::end_sequence(bool restore)
{
    free_sequence();
    if (restore)
        pop_state();
    flags &= ~CAMF_SEQUENCE;
    game_flags &= ~0x800;
}

// 0x479830  Confidence: exact. ret 0x14.
void camera::set_view_params(float dist, float elev, float z, float lh, float eye_off)
{
    distance = dist;
    elevation = elev;
    zoom = z;
    look_height = lh;
    eye_offset_wanted = eye_off;
    orient_mode = 2;
}

// 0x479870  Confidence: exact. ret 0x14. Nothing reads the stored values (camera_reset_default only tests
// the flag).
void camera::set_default_view_params(float dist, float elev, float z, float lh, float eye_off)
{
    default_distance = dist;
    default_elevation = elev;
    default_zoom = z;
    default_look_height = lh;
    default_eye_offset = eye_off;
    flags |= CAMF_DEFAULTS_SET;
}

// 0x4798b0  Confidence: exact (extract_angles(&p, &b, &h); the heading + pi, wrapped to [-pi, pi)).
void camera::sync_yaw_to_target()
{
    float h, b, p;
    float y;

    if (!target)
        return;
    target->orient.extract_angles(&p, &b, &h);
    y = h + CAM_PI;
    while (y >= CAM_PI)
        y -= CAM_TWO_PI;
    while (y < -CAM_PI)
        y += CAM_TWO_PI;
    yaw = y;
}

// 0x479930  Confidence: likely (the mode-4 branch's three exits). ret 4. Mode 4 is "back": to the
// previous mode, or to 3 after a glide that left mode 3. A suppressed forced-high request is only
// remembered. After a change the zoom of that view comes back.
void camera::set_mode(int m)
{
    int new_mode;

    if (mode == m)
        return;
    if (high_view_suppressed && m == 3) {
        high_view_requested = 1;
        return;
    }
    high_view_requested = 0;
    if (m == 4) {
        if (!(flags & CAMF_FOLLOW_TARGET)) {
            flags |= CAMF_FOLLOW_TARGET;
            new_mode = prev_mode;
        } else if (flags & CAMF_BACK_TO_HIGH) {
            flags &= ~CAMF_BACK_TO_HIGH;
            new_mode = 3;
        } else {
            new_mode = prev_mode;
        }
    } else if (m == 3 && !(flags & CAMF_FOLLOW_TARGET)) {
        flags |= CAMF_FOLLOW_TARGET;
        new_mode = m;
    } else {
        new_mode = m;
    }
    if (new_mode == mode)
        return;
    if (mode == 3 || mode == 5) {
        if (mode == 3 && m == 5)
            flags |= CAMF_BACK_TO_HIGH;
    } else {
        prev_mode = mode;
    }
    mode = new_mode;
    if (new_mode == 5)
        return;
    if (new_mode == 1 || new_mode == 3) {
        view_high();
    } else if (new_mode == 0) {
        view_low();
    } else if (new_mode == 2) {
        if (prev_mode != 2 || m == 4) {
            view_low();
            auto_view_timer.SetTimeout(0);
            auto_view_update();
        }
    }
    flags &= ~CAMF_BACK_TO_HIGH;
    if (elevation < 4.0f)
        zoom = zoom_low;
    else
        zoom = zoom_high;
}

// 0x479a60  Confidence: exact (VC6 byte match, cl 8966, match21; was likely; each branch stores prev_mode = mode after set_mode). F12: 0 -> 1 -> 2 -> 0;
// a suppressed forced-high mode 3 counts as 1.
bool camera::cycle_mode()
{
    if (cur_level_index == 4)
        return false;
    int m;

    if (mode == 0)
        m = 1;
    else if (mode == 1 || (high_view_suppressed && mode == 3))
        m = 2;
    else if (mode == 2)
        m = 0;
    else
        return false;
    set_mode(m);
    prev_mode = mode;
    return true;
}

// 0x479ad0  Confidence: exact. ret 8.
void camera::set_sequence_transform(vector3 *origin, matrix3 *orient)
{
    if (origin) {
        seq_origin = *origin;
        seq_orient = *orient;
        flags |= CAMF_SEQ_TRANSFORM;
    } else {
        flags &= ~CAMF_SEQ_TRANSFORM;
    }
}
