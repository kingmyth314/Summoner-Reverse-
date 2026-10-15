// engine/camera/camera_sequence.h - camera sequences (camera_sequence 0x493f20-0x493f73 and the cutscene
// camera shot 0x493f80-0x4946a0; the unit is not known: module "-", no path string). File name and
// directory ours. Not original source. Layout: symbols/types.h cutscene_camera_shot (0x70 bytes).
// decomp52: declared for the cutscene file (unnamed_cutscene_0x434900.cpp: the 50 shots of the cutscene
// object and the camera_start_sequence argument).
#ifndef ENGINE_CAMERA_CAMERA_SEQUENCE_H
#define ENGINE_CAMERA_CAMERA_SEQUENCE_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"

class File;

// The base sequence (vtable 0x581ae4: slot 0 the scalar deleting destructor 0x493f40, slots 1 and 2
// _purecall 0x56e903). camera_start_sequence keeps the running one in camera +0x13c.
class camera_sequence {
public:
    camera_sequence(int state);       // 0x493f20 camera_sequence__ctor (+4 = state)
    virtual ~camera_sequence();       // 0x493f60 camera_sequence__dtor (scalar deleting 0x493f40)
    virtual vector3 *get_pos() = 0;   // slot 1
    virtual matrix3 *get_orient() = 0; // slot 2
    int get_state();                  // 0x493f70 camera_sequence_get_state (returns +4)

    int state;                        // +0x04
};
LAYOUT_CHECK(camera_sequence, sizeof(camera_sequence) == 8);

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
    float time_left();                // 0x4943f0 cutscene_camera_sequence_time_left
    void  advance(float dt);          // 0x494400 cutscene_camera_sequence_advance

    float fov;                        // +0x08
    float start;                      // +0x0C shot start, seconds
    float end;                        // +0x10 shot end, seconds
    ubyte track[0x2c];                // +0x14 anim_track (key counts +0x14 / +0x16, key arrays +0x1c / +0x20)
    vector3 pos;                      // +0x40 sampled position
    matrix3 orient;                   // +0x4C sampled orientation
};
LAYOUT_CHECK(cutscene_camera_sequence, sizeof(cutscene_camera_sequence) == 0x70);

void cutscene_camera_tracks_clear();  // 0x494680 zeroes the camera key pool counts (cutscene::load_files)

#endif
