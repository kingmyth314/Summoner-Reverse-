// vsdk/ca/skeleton.h - animations (.mvf skeletons), key tracks and .vim meshes (vsdk\ca\skeleton.cpp,
// path references 0x543d8c-0x5449da; anim_track 0x551090-0x551350 is in an unnamed unit). Not original
// source. Field names ours unless a symbol comment names them.
#ifndef VSDK_CA_SKELETON_H
#define VSDK_CA_SKELETON_H

#include "vsdk/math/matrix.h"

// One key track of a skeleton (0x2c bytes per bone, skeleton +0x8c).
class anim_track {
public:
    quaternion get_rotation(int time, int flag);      // 0x551350 (flag 0 in the pilots)
    vector3 get_position(int time);                   // 0x551090
    ubyte _data[0x2c];
};
LAYOUT_CHECK(anim_track, sizeof(anim_track) == 0x2c);

struct ca_trigger {                   // 8 bytes, skeleton +0xa8: "left-foot", "right-foot", ...
    const char *name;                 // +0x0 NULL = unused slot
    int time;                         // +0x4 anim time (ticks)
};

// One animation (.mvf); only the fields character_instance.cpp reads.
class skeleton {
public:
    vector3 get_root_velocity();      // 0x543fc0 skeleton__get_root_velocity
    ubyte _unk00[0x5c];
    bool is_state;                    // +0x5C state (looping, locomotion) anim; 0 = action
    bool root_rotation;               // +0x5D the anim turns the character (yaw taken from the root bone)
    ubyte _pad5e[2];
    int start;                        // +0x60 first tick
    int end;                          // +0x64 last tick
    ubyte _unk68[0x20];
    ubyte *bone_weights;              // +0x88 per bone, 0..10
    anim_track *tracks;               // +0x8C per bone
    ubyte _unk90[0x10];
    int fade_in;                      // +0xA0 ticks
    int fade_out;                     // +0xA4 ticks
    ca_trigger triggers[5];           // +0xA8
};

// The .vim mesh header (item meshes: item_paging.cpp; character meshes: ca_mesh +0x28). One type for the
// stubs' vim and ca_vim (DECISIONS.md D21).
struct vim {
    ubyte _unk00[0x10];
    int textures[6];                  // +0x10 bitmap handles
    int num_textures;                 // +0x28
    ubyte _unk2c[0x24];
    vector3 center;                   // +0x50 bounding sphere
    float radius;                     // +0x5C
};
typedef vim ca_vim;

void vim_free(vim *mesh);                         // 0x5430b0
vim *vim_load(const char *name, int arg);          // 0x543920
int  vim_find_tag(vim *mesh, const char *name);    // 0x543c20, -1 if none
// Tag position (+0x54) and orientation (+0x44) of a 0x60-byte tag record (decomp23; argument order from
// combat_fire_weapon_missile, the outputs feed matrix43(pos, rot)).
void vim_get_tag_transform(vim *mesh, int tag, vector3 *pos, matrix *orient);   // 0x543c70
void ca_init_globals_2ee4598();                    // 0x544a10
void ca_close_globals_544a40();                    // 0x544a40 (no name in sum.symbols; label ours)

#endif
