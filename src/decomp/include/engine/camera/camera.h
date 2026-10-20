// engine/camera/camera.h - the game camera (docs/sum-exe/camera.md). decomp58: the camera unit is
// 0x4778b0-0x479b59 (three console commands, then every camera method), written as
// game/unnamed_camera_0x4778b0.cpp; the file name is not known (no path string, no strings.tbl block,
// module "-"), the directory is ours. Not original source. Layout: symbols/types.h struct game_camera, as
// a class so the object game_camera (0x60a018, gameplay.cpp's .bss) can have its own name (DECISIONS.md D19).
#ifndef ENGINE_CAMERA_CAMERA_H
#define ENGINE_CAMERA_CAMERA_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/matrix.h"

class Character;
class obj;
class camera_sequence;
class cutscene_camera_sequence;

// camera::flags bits (+0x0c; names ours, D3802).
#define CAMF_FOLLOW_VIEW    0x001     // the follow eye drives the rendered view (camera_copy_follow_view)
#define CAMF_FOLLOW_TARGET  0x002     // follow the target (input, smoothing, auto view)
#define CAMF_SEQUENCE       0x008     // a camera sequence (cutscene shot) runs
#define CAMF_FREE_LOOK      0x010     // free look / slew (only while CAMF_FOLLOW_VIEW is clear)
#define CAMF_DEFAULTS_SET   0x020     // camera_set_default_view_params ran
#define CAMF_ZOOM_BLOCKED   0x040     // set by camera_auto_rotate_update while it turns
#define CAMF_SEQ_TRANSFORM  0x080     // seq_origin / seq_orient apply to every shot
#define CAMF_RESTORE_ZOOM   0x100     // restore saved_zoom when the auto-turn ends
#define CAMF_BACK_TO_HIGH   0x200     // mode 4 goes back to mode 3 (left mode 3 for mode 5)

class camera {                        // 0x2a8 bytes
public:
    camera();                         // 0x478d70 camera__camera (gameplay.cpp's static game_camera; decomp42)
    ~camera();                        // 0x478df0 camera__dtor (atexit dtor_game_camera)
    void set_target(Character *c);    // 0x478ff0 camera_set_target
    // decomp20 (conversationctl.cpp). (follow target -> flag 0x2, free look -> flag 0x10); flag 0x1 is
    // cleared; a running sequence ends first (restore = !free_look; decomp58).
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
    // decomp58: both arguments are bools (`cmp al, 1` at 0x478e77 and 0x478e9e; was (int, int)).
    void snap_to_target(bool update_target, bool full);   // 0x478e70 camera_snap_to_target (1, 1)
    void sync_yaw_to_target();        // 0x4798b0 camera_sync_yaw_to_target
    // Inline accessors of the original header (DECISIONS.md C3); conversationctl.cpp calls them as the
    // COMDATs 0x4d8f30 game_camera__get_pos / 0x4d8f40 game_camera__get_orient (lea eax, [ecx + off]).
    vector3 &get_pos() { return eye_pos; }
    matrix3 &get_orient() { return eye_orient; }
    // F12: 0 -> 1 -> 2 -> 0. decomp58: returns AL (false on the world map and in modes 3 / 4 / 5; was void).
    bool cycle_mode();                // 0x479a60 camera_cycle_mode
    // (position, orientation, distance, two floats): an empty body (ret 0x14). The level scripts pass a
    // navpoint's position and matrix, 17.0 and 2.0, 2.0 (lsf_lenele_flag_cart_played). decomp46.
    void cut_to_view_stub(vector3 *pos, matrix3 *orient, float distance, float a, float b);   // 0x479430
    // decomp52 (the player input unit 0x4343a0 and the cutscene file 0x434900):
    void frame();                     // 0x478990 camera_frame (player_input_frame, every game frame)
    void set_input(float yaw, float zoom, float elevation);   // 0x479170 -> +0x23c / +0x240 / +0x244
    // decomp58: the argument is a cutscene shot (cutscene_camera_sequence_sample is called on it directly
    // and its +0x08 fov is read; was camera_sequence *).
    void start_sequence(cutscene_camera_sequence *seq);        // 0x4792c0 camera_start_sequence
    void end_sequence(bool restore);  // 0x4797f0 camera_end_sequence (bool: mov al, [esp + 8])
    // (origin, rotation): applied to every cutscene shot while +0xc bit 0x80 is set; NULL clears.
    void set_sequence_transform(vector3 *origin, matrix3 *orient);   // 0x479ad0
    // decomp58: the rest of the unit, in address order.
    void apply_input_deltas();        // 0x477af0 camera_apply_input_deltas
    void auto_rotate_update();        // 0x477ca0 camera_auto_rotate_update
    vector3 compute_eye_pos();        // 0x477ea0 camera_compute_eye_pos (hidden result, ret 4)
    vector3 get_target_offset();      // 0x477f10 camera_get_target_offset (hidden result, ret 4)
    void update_orientation();        // 0x477f60 camera_update_orientation
    void update_eye_position();       // 0x478130 camera_update_eye_position
    // (unused, level object index, unused, unused): thiscall, ret 0x10; only the index is read.
    void add_occluder(vector3 *a, int lobj_index, int start_count, float radius);   // 0x4784e0
    // (a, b, ray radius): two rays (b -> a, then a -> b) against the draw lists; 1 when anything was found.
    bool find_occluders(vector3 *a, vector3 *b, float radius);  // 0x478520 camera_find_occluders (ret 0xc; bool: D12407)
    void smooth_height(float h);      // 0x478700 camera_smooth_height
    void auto_view_update();          // 0x4787b0 camera_auto_view_update
    void view_high();                 // 0x478930 camera_view_high
    void view_low();                  // 0x478960 camera_view_low
    void update_target_pos();         // 0x478e00 camera_update_target_pos
    void set_height(float h);         // 0x478fd0 camera_set_height
    void copy_follow_view();          // 0x4790d0 camera_copy_follow_view
    void free_look_update();          // 0x4791a0 camera_free_look_update
    void free_sequence();             // 0x479440 camera_free_sequence
    void update_sequence();           // 0x479480 camera_update_sequence
    bool enemy_near_leader();         // 0x4796a0 camera_enemy_near_leader (thiscall, this unused, no argument)
    void set_view_params(float distance, float elevation, float zoom, float look_height,
                         float eye_offset);                // 0x479830 camera_set_view_params
    void set_default_view_params(float distance, float elevation, float zoom, float look_height,
                                 float eye_offset);        // 0x479870 camera_set_default_view_params

    int mode;            // +0x000 0 low, 1 high, 2 automatic, 3 forced high, 5 glide (camera_set_mode)
    int prev_mode;       // +0x004
    ubyte high_view_suppressed; // +0x008 from the detail level (detail_apply_level_settings)
    ubyte high_view_requested;  // +0x009 a mode-3 request remembered while +0x008 is set
    ubyte _unk00a[2];
    uint flags;          // +0x00C CAMF_* (1 follow drives the view, 2 follow target, 8 sequence, 0x10 free look, 0x20 default params stored, 0x40 block zoom input (set and cleared by camera_auto_rotate_update), 0x80 sequence transform, 0x100 restore zoom, 0x200 back to mode 3)
    uint num_saved_states; // +0x010 state stack depth, max 8 (unsigned: jae / jbe at 0x4797b9 / 0x4797d5; decomp58)
    uint saved_states[8][9]; // +0x014 9 dwords each (camera_get_state: flags, distance, yaw, zoom, elevation, fov, look height, mode, prev mode)
    float   zoom_high;       // +0x134 zoom remembered for the high view (start 1.0)
    float   zoom_low;        // +0x138 zoom remembered for the low view (start 0.8)
    camera_sequence *sequence; // +0x13C running camera sequence (a cutscene shot), virtual dtor (was void *)
    vector3 look_at;         // +0x140 target position + centre offset, y + look_height (camera_update_target_pos)
    obj    *target;          // +0x14C followed object (pos +0x10 and centre offset +0x28 are read through it; was void *)
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

// ---- the camera unit's globals and free functions (decomp58) ---------------------------------------
// (value, velocity, acceleration, dt, target): one integration step that snaps to the target (velocity
// 0) when it crosses it. cdecl.
void camera_approach_value(float *value, float *velocity, float accel, float dt, float target);  // 0x478450
extern float mlook_sensitivity;       // 0x59b7fc 1.0 ("mlook_sensitivity")
extern float camera_zoom_max;         // 0x59b800 1.0 ("zoom_limit": 1.0 / 1000.0)
extern float camera_eye_offset_speed; // 0x59b804 1.5 per second (name ours)
#define MAX_CAMERA_OCCLUDERS 32
extern int camera_occluders[MAX_CAMERA_OCCLUDERS];   // 0x22e49b8 level object indices (levelrender_fade_occluders)
extern int camera_num_occluders;      // 0x22e4aec
extern int camera_occluders_unk;      // 0x22e4acc zeroed with the count, read by camera_find_occluders, never set (name ours)
extern bool Camera_geometry_avoidance;   // 0x22e4b0c ("geom_avoid"; nothing reads it)
void cmd_zoom_limit();                // 0x4778e0
void cmd_geom_avoid();                // 0x477990
void cmd_mlook_sensitivity();         // 0x477a80

#endif
