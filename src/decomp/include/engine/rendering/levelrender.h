// engine/rendering/levelrender.h - level rendering and level geometry (engine\rendering\levelrender.cpp,
// path references 0x48e4d9-0x4914c6; levelrender_render_scene 0x482200, options_apply_detail 0x4872d0 and
// the level object arrays are placed here by address and use). Not original source.
#ifndef ENGINE_RENDERING_LEVELRENDER_H
#define ENGINE_RENDERING_LEVELRENDER_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/timestamp.h"
#include "engine/s3d/s3d.h"

class Character;

// One face reference of a level object (8 bytes, lobj +0x0c, one per geometry face; s3d_read_mesh_ref):
// a 1-based index into level_texture_bms (0 untextured), and a 24-bit lightmap index into
// level_face_bitmaps (-1 none) packed with a signed byte. The bit fields are decomp28's (D801): the loader
// reads the low 24 bits with shl 8 / sar 8 and the byte with sar 24, and stores both with one or / and.
struct lobj_face {
    ushort texture;                   // +0x00
    ushort _pad2;                     // +0x02
    int lightmap : 24;                // +0x04
    int _unk7 : 8;                    // +0x07 written as an int16 in the file
};
// Level objects (0x70 bytes each; s3d_read_meshes). The other fields are the ones new_hud.cpp reads
// (hud_interact_nearest; merged with gamesound.cpp's partial copy, DECISIONS.md D51). Flags set by the
// loader: 0x10 a face texture has alpha, 0x4000 named by a $vis set (cleared after loading), 0x8000 no
// textured face, 0x100000 / 0x200000 / 0x400000 / 0x800000 / 0x1000000 per-name hacks, 0x80000 lightmap
// off (console lmap_off / lmap_reset).
struct s3d_geometry;
struct s3d_box;
struct lobj {
    lobj();                           // 0x4775e0 lobj_clear_layers (layers = -1; returns this; D803)
    int pick_layer(const vector3 *pos, int r);   // 0x477600 lobj_pick_layer (thiscall; objects.cpp passes 1)
    int get_layer_n(int n);           // 0x4775f0 lobj_get_layer_n (signed byte layers[n])
    void set_layer_n(int n, int layer);   // 0x4776a0 lobj_set_layer_n
    char *name;                       // +0x00
    s3d_geometry *mesh;               // +0x04
    unsigned int flags;               // +0x08 0x100 door, 0x80 not a container (hud_interact_nearest, objects_update_frame); tested against the ray masks
    lobj_face *faces;                 // +0x0c
    s3d_box *lightmap_boxes;          // +0x10 one per face when the first face has a lightmap
    vector3 pos;                      // +0x14 position used for containers (lc_entries)
    matrix orient;                    // +0x20 (tutorial_cond_combat_part2 writes it from a door quaternion; D100)
    vector3 centre;                   // +0x44 position used for doors
    vector3 bbox_min;                 // +0x50 (lobj_get_bbox copies +0x50 and +0x5c)
    vector3 bbox_max;                 // +0x5c
    ubyte *vertex_rgb;                // +0x68 three bytes per geometry vertex (byte pool)
    signed char layers[4];            // +0x6c two layer bytes and two more (lobj() sets all four to -1)
};
LAYOUT_CHECK(lobj, sizeof(lobj) == 0x70);
// num_lobjs / lobjs are fields of level_s3d now (macros in engine/s3d/s3d.h, D800).

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
// A door record (0x8c bytes, sorted by lobj index at 0xa5a304; door_update reads both poses). Type and
// field names ours (D100). s3d_read_door reads +0x00..+0x43 and resets the rest (decomp28, D802).
struct door_info {
    int lobj_index;                   // +0x00
    short named_path;                 // +0x04 level named path the door takes its name from, -1 none
    short _unk06;                     // +0x06
    float _unk08;                     // +0x08
    vector3 closed_pos;               // +0x0c door_update reads +0x10 and +0x1c too
    quaternion closed_orient;         // +0x18
    vector3 open_pos;                 // +0x28
    quaternion open_orient;           // +0x34
    GameTimer timer;                  // +0x44 move timer (constructed by new door_info[n])
    float _unk48[3];                  // +0x48
    unsigned int state;               // +0x54 bits 0, 1 and 3 cleared on load; bit 3 the default locked
                                      //        state (savegame_two_stage_get, decomp27)
    float lock;                       // +0x58 lock value, 0 unlocked (character_use_skill pick lock; D100)
    int _unk5c;                       // +0x5c
    int _unk60;                       // +0x60
    int _unk64;                       // +0x64 -1 on load
    ubyte _unk68[4];                  // +0x68 three bytes cleared on load
    int _unk6c;                       // +0x6c
    int _unk70;                       // +0x70 -1 on load
    ubyte _unk74[4];                  // +0x74 three bytes cleared on load
    ubyte _unk78[0x14];               // +0x78
};
LAYOUT_CHECK(door_info, sizeof(door_info) == 0x8c);
door_info *door_from_lobj(lobj *l);   // 0x4768c0 0 unless the lobj has flag 0x100
// (door, fraction): open, or moving with the eased progress past fraction (pf_door_index_is_open passes
// 0.4f; decomp32). Returns AL.
bool door_is_open(door_info *d, float fraction);   // 0x476940
// The door commands' helpers (unit not known; decomp27). door_find_by_name: 0 found, -1 not (the outputs
// are then not written).
int  door_find_by_name(const char *name, lobj **out_lobj, door_info **out_door);   // 0x477280
bool door_set_open(lobj *l, float speed, int a, int b);   // 0x476e00 (cmd_door_open: -1.0, 1, 1)
void door_set_closed(lobj *l);        // 0x476e80 (return unused)
void door_toggle_state(lobj *l, int a);   // 0x476ee0 (return unused; cmd_door_toggle passes 1)
lobj *lobj_from_index(int index);     // 0x477780
int  lobj_find_by_name(const char *name);           // 0x4777a0 -1 if none
void fx_disable_by_effect_name(const char *name);   // 0x48d660 (unit not known; effect objects)
void lobj_get_bbox(int index, vector3 *mins, vector3 *maxs);   // 0x4776c0
// Door objects within reach of the character (unit not known; next to the AI code).
bool door_in_use_range(Character *c, lobj *door);             // 0x476a30

void levelrender_render_scene();      // 0x482200
void options_apply_detail();          // 0x4872d0 (detail settings; name from sum.symbols)
void levelrender_clear_draw_lists();  // 0x486b00

// (lobj, face, point, out r, g, b): writes 0xff to the three output bytes (unit not known; decomp23).
void lighting_sample_stub(int lobj, int face, const float *point, ubyte *r, ubyte *g, ubyte *b);  // 0x4af320
// Shadows and level effects (0x4874c0 / 0x48a060 / 0x48a0e0, between options_apply_detail and the
// experience table; unit not known). Called by summoner.cpp's GameInit / game_cleanup.
void shadows_parse_table();           // 0x4874c0 shadows.tbl
void shadows_init();                  // 0x48a060 lamp glow, Shadow01.tga
void fx_shutdown();                   // 0x48a0e0
// S3d_load_lightmaps (0x59aa58) moved to engine/s3d/s3d.h (s3d.cpp .data).

// levelrender.cpp (0x48db30-0x492af0, decomp28).
struct gr_vertex;
struct gr_light;
extern float level_light_scale;       // 0x59c358 1.0 (the Luminar fight changes it)
extern float Small_mesh_threshold;    // 0x59c47c console "small_mesh_threshold"
extern float Time_mult;               // 0x59c480 console "time_mult" (wave speed)
extern float Space_mult;              // 0x59c484 console "space_mult" (wave noise scale)
extern void *levelrender_d3d_vertex_buffer;   // 0x243e03c IDirect3DVertexBuffer7*
void levelrender_wave_lobj(lobj *l, vector3 *out);              // 0x48de60
void levelrender_create_vertex_buffer();                        // 0x48e470
void levelrender_free_d3d_vertex_buffer();                      // 0x48e4f0
void level_lights_setup();                                      // 0x48e520
// Fills the three static pipeline vertices of a face, clips and projects them: (lobj, face, unused,
// lightmap uvs (u0, u1, u2, v0, v1, v2) or NULL, clip codes per vertex, view-space vertices, out count,
// vertex light colours, use the colours). Returns the vertex list.
gr_vertex **levelrender_face_vertex_colors(lobj *l, s3d_face *face, int unused, float *lightmap_uv,
                                           ubyte *codes, vector3 *verts, int *num_out,
                                           vector3 *colors, int lit);   // 0x48f8c0
// (out rgb, point, normal, count, lights): the clamped sum of the lights at a point; returns how many
// lit it.
int  levelrender_sum_point_lights(vector3 *out, const vector3 *pos, const vector3 *normal, int count,
                                  gr_light **lights);           // 0x48fbb0

// Level object helpers (unit not known, next to door_from_lobj).
s3d_textured_record *lobj_find_textured_record(int lobj_index);   // 0x477450 bsearch of level +0x70
int  name_ptr_stricmp_compare(const void *a, const void *b);      // 0x4768a0 qsort / bsearch comparator
// Ken Perlin's reference noise over a vector (unit not known; the vfx code calls it too).
float perlin_noise3(vector3 v);       // 0x5470e0

#endif
