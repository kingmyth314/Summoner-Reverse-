// engine/rendering/levelrender.h - level rendering and level geometry (engine\rendering\levelrender.cpp,
// path references 0x48e4d9-0x4914c6; levelrender_render_scene 0x482200, options_apply_detail 0x4872d0 and
// the level object arrays are placed here by address and use). Not original source.
#ifndef ENGINE_RENDERING_LEVELRENDER_H
#define ENGINE_RENDERING_LEVELRENDER_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

class Character;

// Level objects (0x70 bytes each); +0xC points at 8-byte face records whose first word is a 1-based
// index into level_texture_bms. The other fields are the ones new_hud.cpp reads (hud_interact_nearest;
// merged with gamesound.cpp's partial copy, DECISIONS.md D51).
struct lobj_face {
    ushort texture;
    ushort _pad2;
    int _unk4;
};
struct lobj {
    int pick_layer(const vector3 *pos, int r);   // 0x477600 lobj_pick_layer (thiscall; objects.cpp passes 1)
    char *name;                       // +0x00
    void *mesh;                       // +0x04
    unsigned int flags;               // +0x08 0x100 door, 0x80 not a container (hud_interact_nearest, objects_update_frame); tested against the ray masks
    lobj_face *faces;                 // +0x0c
    int _unk10;                       // +0x10
    vector3 pos;                      // +0x14 position used for containers (lc_entries)
    char _pad20[0x24];
    vector3 centre;                   // +0x44 position used for doors
    vector3 bbox_min;                 // +0x50 (lobj_get_bbox copies +0x50 and +0x5c)
    vector3 bbox_max;                 // +0x5c
    char _pad68[0x08];                // +0x68 (+0x6c: two layer bytes, sum.symbols)
};
LAYOUT_CHECK(lobj, sizeof(lobj) == 0x70);
extern int num_lobjs;                 // 0xa5a2c8
extern lobj *lobjs;                   // 0xa5a2cc

// A ray for lobj_raycast (0x4c bytes; layout from item_obj_spawn and lobj_raycast 0x4ae890; type and
// field names ours). The inline constructor clears +0x30: item_obj_spawn stores 0 there before its first
// statement.
struct lobj_ray {
    lobj_ray() { _unk30 = 0; }
    vector3 start;                    // +0x00
    vector3 dir;                      // +0x0C length = reach
    float _unk18;                     // +0x18
    int flags;                        // +0x1C bit 1 selects lobj_ray_test_geometry
    unsigned int require_flags;       // +0x20 lobj flags that must all be set
    unsigned int exclude_flags;       // +0x24 lobj flags that must be clear
    int _unk28;                       // +0x28
    int _unk2c;                       // +0x2C
    int _unk30;                       // +0x30
    vector3 local_start;              // +0x34 written by lobj_raycast (ray in the object frame)
    vector3 local_dir;                // +0x40
};
LAYOUT_CHECK(lobj_ray, sizeof(lobj_ray) == 0x4c);

// The hit record (0x2c bytes; names ours). lobj_raycast clears +0x00, +0x20, +0x28 when its flag is 1.
struct lobj_hit {
    int num_hits;                     // +0x00
    float t;                          // +0x04 hit distance as a fraction of ray.dir
    int _unk08[6];                    // +0x08
    int _unk20;                       // +0x20
    int face;                         // +0x24 copied to item_obj +0xa8 (meaning unsure)
    int _unk28;                       // +0x28
};
LAYOUT_CHECK(lobj_hit, sizeof(lobj_hit) == 0x2c);

// Level object ray tests (unit 0x4ae330-0x4b0210, file not known).
void lobj_raycast(int lobj_index, lobj_ray *ray, lobj_hit *hit, bool clear);   // 0x4ae890
void lobj_raycast_all(lobj_ray *ray, lobj_hit *hit);                           // 0x4af2c0
void door_update(lobj *door);         // 0x477080 (unit not known)
extern int *level_texture_bms;        // 0xa5a320
void lobj_get_bbox(int index, vector3 *mins, vector3 *maxs);   // 0x4776c0
// Door objects within reach of the character (unit not known; next to the AI code).
bool door_in_use_range(Character *c, lobj *door);             // 0x476a30

void levelrender_render_scene();      // 0x482200
void options_apply_detail();          // 0x4872d0 (detail settings; name from sum.symbols)
void levelrender_clear_draw_lists();  // 0x486b00

#endif
