// engine/camera/camera_sequence.h - camera sequences (camera_sequence 0x493f20-0x493f73 and the cutscene
// camera shot 0x493f80-0x4946a0; the unit is not known: module "-", no path string). File name and
// directory ours. Not original source. Layout: symbols/types.h cutscene_camera_shot (0x70 bytes).
// decomp52: declared for the cutscene file (unnamed_cutscene_0x434900.cpp: the 50 shots of the cutscene
// object and the camera_start_sequence argument).
// decomp54: written out in game/unnamed_cutscene_camera_0x493e50.cpp. time_left and advance are not
// cutscene shot members: they belong to a second derived class (state 1) whose fields differ
// (camera_blend_sequence below, D3401).
#ifndef ENGINE_CAMERA_CAMERA_SEQUENCE_H
#define ENGINE_CAMERA_CAMERA_SEQUENCE_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/skeleton.h"

class File;
class camera;

// The base sequence (vtable 0x581ae4: slot 0 the scalar deleting destructor 0x493f40, slots 1 and 2
// _purecall 0x56e903). camera_start_sequence keeps the running one in camera +0x13c.
// state: 0 a cutscene shot (camera_update_sequence samples it), 1 a blend (advanced by the frame time,
// camera_frame ends it when time_left reaches 0).
class camera_sequence {
public:
    camera_sequence(int state);       // 0x493f20 camera_sequence__ctor (+4 = state)
    virtual ~camera_sequence();       // 0x493f60 camera_sequence__dtor (scalar deleting 0x493f40)
    virtual vector3 *get_pos() = 0;   // slot 1
    virtual matrix3 *get_orient() = 0; // slot 2
    int get_state();                  // 0x493f70 camera_sequence_get_state (returns +4)

    int state;                        // +0x04
};
LAYOUT_CHECK_32(camera_sequence, sizeof(camera_sequence) == 8);

// One cutscene camera shot (vtable 0x581af0; cutscene_camera_sequence__ctor builds it on
// camera_sequence(0)). Read from a .csc file by read_track; start / end are seconds from the start of
// the cutscene (each shot starts where the previous one ended: cutscene::load_files).
class cutscene_camera_sequence : public camera_sequence {
public:
    cutscene_camera_sequence();       // 0x493f80 cutscene_camera_sequence__ctor
    ~cutscene_camera_sequence();      // 0x493fc0 cutscene_camera_sequence__dtor (scalar deleting 0x493fa0)
    vector3 *get_pos();               // 0x4943d0 (this + 0x40)
    matrix3 *get_orient();            // 0x4943e0 (this + 0x4c)
    void  read_track(File *f, float start);   // 0x493fd0 cutscene_camera_read_track
    void  sample();                   // 0x494310 cutscene_camera_sequence_sample

    float fov;                        // +0x08 file value * 1.6949153 (camera_update_sequence: x 1/pi)
    float start;                      // +0x0C shot start, seconds
    float end;                        // +0x10 shot end, seconds
    anim_track track;                 // +0x14 key counts +0x14 / +0x16, key arrays +0x1c / +0x20 (decomp54:
                                      //       was ubyte[0x2c])
    vector3 pos;                      // +0x40 sampled position
    matrix3 orient;                   // +0x4C sampled orientation
};
LAYOUT_CHECK_32(cutscene_camera_sequence, sizeof(cutscene_camera_sequence) == 0x70);

// A blend from the camera's own view to a fixed view (state 1; name and field names ours, D3401). No
// constructor, vtable or creation is in Sum.exe, only the two methods camera_frame and
// camera_update_sequence call (sum.symbols names them cutscene_camera_sequence_*). The weight ramps
// up over blend_in, holds, and ramps down over blend_out (tcb_ease 0.25 / 0.25).
class camera_blend_sequence : public camera_sequence {
public:
    float time_left();                // 0x4943f0 camera_blend_sequence__time_left
    void  advance(float dt);          // 0x494400 camera_blend_sequence__advance (ret 4)

    float elapsed;                    // +0x08
    float hold;                       // +0x0C
    float blend_in;                   // +0x10
    float blend_out;                  // +0x14
    vector3 pos;                      // +0x18 blended position (get_pos)
    matrix3 orient;                   // +0x24 blended orientation (get_orient)
    camera *cam;                      // +0x48 eye_pos (+0x1a4) and the sequence orientation (+0x284)
    vector3 target_pos;               // +0x4C
    matrix3 target_orient;            // +0x58
};
LAYOUT_CHECK_32(camera_blend_sequence, sizeof(camera_blend_sequence) == 0x7c);

void cutscene_camera_tracks_clear();  // 0x494680 zeroes the camera key pool counts (cutscene::load_files)

// The key pools of the shots (decomp54; sum.symbols names, typed by the static initialisers).
#define CUTSCENE_CAM_POS_KEYS 250
#define CUTSCENE_CAM_ROT_KEYS 250
extern anim_pos_key cutscene_cam_keys40[CUTSCENE_CAM_POS_KEYS];       // 0x243e3e8
extern anim_rot_key cutscene_cam_keys20[CUTSCENE_CAM_ROT_KEYS];       // 0x2440af8
extern vector3 cutscene_cam_keys12[2 * CUTSCENE_CAM_POS_KEYS];        // 0x2441e80
extern cquat cutscene_cam_keys8[2 * CUTSCENE_CAM_ROT_KEYS];           // 0x24435f8
extern int cutscene_cam_keys8_count;                                  // 0x243e3d8
extern int cutscene_cam_keys40_count;                                 // 0x243e3dc
extern int cutscene_cam_keys20_count;                                 // 0x243e3e0
extern int cutscene_cam_keys12_count;                                 // 0x24435f0

#endif
