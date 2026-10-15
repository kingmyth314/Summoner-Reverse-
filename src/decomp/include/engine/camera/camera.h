// engine/camera/camera.h - the game camera (camera methods 0x478ff0..; docs/sum-exe/camera.md; unit and
// directory not known). Not original source. Layout: symbols/types.h struct game_camera, as a class so
// the object game_camera (0x60a018) can have its own name (DECISIONS.md D19).
#ifndef ENGINE_CAMERA_CAMERA_H
#define ENGINE_CAMERA_CAMERA_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/matrix.h"

class Character;

class camera {                        // 0x2a8 bytes
public:
    camera();                         // 0x478d70 camera__camera (gameplay.cpp's static game_camera; decomp42)
    ~camera();                        // 0x478df0 camera__dtor (atexit dtor_game_camera)
    void set_target(Character *c);    // 0x478ff0 camera_set_target
    // decomp20 (conversationctl.cpp):
    void set_follow_flags(bool a, bool b); // 0x479740 camera_set_follow_flags
    void resume_follow();             // 0x479790 camera_resume_follow
    void push_state();                // 0x4797b0 camera_push_state
    void pop_state();                 // 0x4797d0 camera_pop_state
    // decomp27 (the CMRA chunk): 9 dwords {flags, distance, yaw, zoom, elevation, fov, look height, mode,
    // prev mode} (sum.symbols camera_get_state).
    void get_state(uint *out);        // 0x478890 camera_get_state
    void set_state(const uint *in);   // 0x4788e0 camera_set_state
    void reset_default();             // 0x478f60 camera_reset_default
    void set_mode(int mode);          // 0x479930 camera_set_mode
    void snap_to_target(int update_target, int arg);   // 0x478e70 camera_snap_to_target (1, 1)
    void sync_yaw_to_target();        // 0x4798b0 camera_sync_yaw_to_target
    // Inline accessors of the original header (DECISIONS.md C3); conversationctl.cpp calls them as the
    // COMDATs 0x4d8f30 game_camera__get_pos / 0x4d8f40 game_camera__get_orient (lea eax, [ecx + off]).
    vector3 &get_pos() { return eye_pos; }
    matrix3 &get_orient() { return eye_orient; }
    void cycle_mode();                // 0x479a60 camera_cycle_mode (F12: 0 -> 1 -> 2 -> 0)

    int mode;            // +0x000 0 low, 1 high, 2 automatic, 3 forced high, 5 glide (camera_set_mode)
    int prev_mode;       // +0x004
    ubyte high_view_suppressed; // +0x008 from the detail level (detail_apply_level_settings)
    ubyte high_view_requested;  // +0x009 a mode-3 request remembered while +0x008 is set
    ubyte _unk00a[2];
    uint flags;          // +0x00C 1 follow drives the view, 2 follow target, 8 sequence, 0x10 free look, 0x20 default params stored, 0x40 block zoom input (set and cleared by camera_auto_rotate_update), 0x80 sequence transform, 0x100 restore zoom, 0x200 back to mode 3
    int num_saved_states;// +0x010 state stack depth, max 8 (camera_push_state / camera_pop_state)
    uint saved_states[8][9]; // +0x014 9 dwords each (camera_get_state: flags, distance, yaw, zoom, elevation, fov, look height, mode, prev mode)
    float   zoom_high;       // +0x134 zoom remembered for the high view (start 1.0)
    float   zoom_low;        // +0x138 zoom remembered for the low view (start 0.8)
    void*   sequence;        // +0x13C running camera sequence (a cutscene shot), virtual dtor
    vector3 look_at;         // +0x140 target position + centre offset, y + look_height (camera_update_target_pos)
    void*   target;          // +0x14C followed object (pointer: pos +0x10 and centre offset +0x28 are read through it)
    vector3 fixed_eye;       // +0x150 eye position glided to in mode 5 (camera_set_target)
    vector3 wanted_dir;      // +0x15C wanted direction for auto-turning (camera_auto_rotate_update; world_pick_under_mouse writes it)
    int orient_mode;     // +0x168 2 = set the view directly (camera_set_view_params, target moving), else smoothed
    float   default_distance;   // +0x16C camera_set_default_view_params, same order as camera_set_view_params
    float   default_elevation;  // +0x170
    float   default_zoom;       // +0x174
    float   default_look_height;// +0x178
    float   default_eye_offset; // +0x17C
    float   yaw;             // +0x180 radians, wrapped
    float   zoom;            // +0x184 clamped to [0.3, camera_zoom_max]
    float   elevation;       // +0x188 eye height above the target per unit of zoom, clamped to [-2, 9]
    float   fov;             // +0x18C vertical, degrees (73.74 by default)
    float   distance;        // +0x190
    float   look_height;     // +0x194 look-at height offset
    float   eye_offset;      // +0x198 eye height offset, current
    float   eye_offset_wanted; // +0x19C eye height offset, wanted (moves at 1.5 per second)
    float   saved_zoom;      // +0x1A0 zoom before auto-turning zoomed out, restored with flag 0x100
    vector3 eye_pos;         // +0x1A4 follow camera eye
    matrix3 eye_orient;      // +0x1B0 follow camera orientation
    GameTimer auto_view_timer; // +0x1D4 GameTimer: 1 s checks, 5 s back to low (camera_auto_view_update)
    ubyte enemy_near;      // +0x1D8 last camera_enemy_near_leader result
    ubyte _pad1d9[3];
    vector3 dir_history[4];  // +0x1DC last target offsets, averaged by camera_update_orientation
    float   dt_history[4];   // +0x20C frame time of each
    int history_index;   // +0x21C ring index (mod 4)
    int history_count;   // +0x220 up to 4
    vector3 eye_velocity;    // +0x224 per-axis velocity of camera_approach_value (zeroed by camera_snap_to_target)
    int _unk230;         // +0x230 zeroed by camera_snap_to_target
    float   height[2];       // +0x234 / +0x238 target height, smoothed / wanted (camera_smooth_height)
    float   input_yaw;       // +0x23C this frame's input rates (camera_set_input)
    float   input_zoom;      // +0x240
    float   input_elevation; // +0x244 always 0 in retail (camera_key_extra_axis)
    vector3 seq_origin;      // +0x248 camera_set_sequence_transform (used with flag 0x80)
    matrix3 seq_orient;      // +0x254
    vector3 view_pos;        // +0x278 rendered view = camera_view_pos (0x60a290)
    matrix3 view_orient;     // +0x284 camera_view_orient (0x60a29c)
};
LAYOUT_CHECK(camera, sizeof(camera) == 0x2a8);
extern camera game_camera;            // 0x60a018

// Parts of game_camera the pilots read under their own symbols (sum.symbols names them). Macros now that
// gameplay.cpp defines game_camera: separate externs inside the object would have no definition (D2205).
#define camera_view_pos (game_camera.view_pos)          // 0x60a290
#define camera_view_orient (game_camera.view_orient)    // 0x60a29c
#define camera_fov (game_camera.fov)                    // 0x60a1a4 (name ours)
#define game_camera_heading (game_camera.yaw)           // 0x60a198 (name ours, new_automap.cpp's camera icon)
// new_hud.cpp's wrapper spelling (the old stubs could not name game_camera; D49).
#define CAMERA_HM (&game_camera)

// decomp42 (gameplay.cpp's static game_camera and its level hooks; free functions, no this).
void camera_level_reset();            // 0x478d20 (level_load_files)
void camera_leave_level();            // 0x478d40 (state_game_leave)

#endif
