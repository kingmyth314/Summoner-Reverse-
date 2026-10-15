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
#include "engine/gamesound/gamesound.h"   // sound_ref (door_info, decomp41)

class Character;
struct ItemType;
class obj;

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
    vector3 pos;                      // +0x08 the hit point (character_probe_lobj_ground; decomp37, was _unk08[6])
    int _unk14[3];                    // +0x14
    int _unk20;                       // +0x20
    int face;                         // +0x24 the level object index (world_pick_lobj, level_scripts.cpp's
                                      //        lobj_from_index(hit.face); character_probe_lobj_ground reads it
                                      //        as a word); copied to item_obj +0xa8
    union {
        int _unk28;                   // +0x28 the s3d_face hit (world_pick_lobj: minus mesh->faces = face index)
        s3d_face *face_ptr;           //       the same as a pointer (geometry face record, 0x14 bytes; decomp37)
    };
};
LAYOUT_CHECK(lobj_hit, sizeof(lobj_hit) == 0x2c);

// Level object ray tests (unit 0x4ae330-0x4b0210, file not known).
void lobj_raycast(int lobj_index, lobj_ray *ray, lobj_hit *hit, bool clear);   // 0x4ae890
void lobj_raycast_all(lobj_ray *ray, lobj_hit *hit);                           // 0x4af2c0
// decomp37 (living_entity.cpp):
void lobj_raycast_draw_lists(lobj_ray *ray, lobj_hit *hit, int *lobj_index, bool *blocked, int a);   // 0x4a4bb0
int  lobj_get_layer(int lobj_index);                  // 0x4775c0 the first layer byte
float char_fog_get_far();                             // 0x481e50 the +CharFog far distance, 0 when off (unit not known)
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
    union {                           // +0x5c the key item type, NULL none (object_try_unlock_or_open;
        int _unk5c;                   //        decomp35, D1505). s3d_read_door clears _unk5c.
        ItemType *key;
    };
    // +0x60 / +0x6c: the "+Open sound:" / "+Close sound:" sound_refs of level_script_parse_table (name
    // from the level script string pool, handle -1, three parameter bytes; decomp41, D2103). s3d.cpp
    // resets them through the old names.
    union {
        struct {
            int _unk60;               // +0x60
            int _unk64;               // +0x64 -1 on load
            ubyte _unk68[4];          // +0x68 three bytes cleared on load
            int _unk6c;               // +0x6c
            int _unk70;               // +0x70 -1 on load
            ubyte _unk74[4];          // +0x74 three bytes cleared on load
        };
        struct {
            sound_ref open_sound;     // +0x60
            sound_ref close_sound;    // +0x6c
        };
    };
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
// false when the door does not move (locked); object_try_unlock_or_open tests AL (D1504).
bool door_toggle_state(lobj *l, int a);   // 0x476ee0 (cmd_door_toggle passes 1, gametarget 0)
lobj *lobj_from_index(int index);     // 0x477780
// Level-script door and object helpers (unit not known, after lobj_from_index; decomp34, D1403).
// door_apply_saved_state: every door with that name whose saved two-stage state differs from `open` starts
// moving (door_start_move(door, -1.0, 1, 1)); the state is compared as a byte (0x47739b).
void door_apply_saved_state(const char *name, bool open);   // 0x477340 (return unused)
bool door_is_open_by_name(const char *name, float fraction); // 0x4773d0 first door of that name, door_is_open
void lobjs_hide_by_prefix(const char *prefix);   // 0x4777f0 sets lobj flag 0x80 (_strnicmp prefix)
void lobjs_show_by_prefix(const char *prefix);   // 0x477850 clears it
int  lobj_find_by_name(const char *name);           // 0x4777a0 -1 if none
void fx_disable_by_effect_name(const char *name);   // 0x48d660 (unit not known; effect objects)
void lobj_get_bbox(int index, vector3 *mins, vector3 *maxs);   // 0x4776c0
// decomp41 (level_scripts_common.cpp). level_marker_find: a level-file navpoint (level_navpoints, _stricmp)
// with its position and orientation, false when none; level_trigger_record_find: a named path
// (level +0x44) or NULL.
bool level_marker_find(const char *name, vector3 *pos, matrix *orient);   // 0x4774a0
s3d_named_path *level_trigger_record_find(const char *name);              // 0x477580
// Fog (unit not known): the level colour, the per-layer distances and colours (20 layers), the snow flag,
// the level default distances (set once per level) and the character fog of "+CharFog:". Which distance
// is near and which far is not settled: fog_set_level_default stores its first argument at 0x59bd88
// (sum.symbols fog_level_near_distance since f7229e0) and level_parse_table passes "+Fog Far:" first.
// fog_set_layer stores a into level_layer_fog_far, b into level_layer_fog_near (sum.symbols names).
void fog_set_color(int r, int g, int b);              // 0x4870e0
void fog_set_layer(int layer, float a, float b, int r, int g, int bl);   // 0x487100 (bytes stored)
void fog_layer_set_snow(int layer, bool snow);        // 0x4871f0
void fog_set_level_default(float a, float b);         // 0x486b90 a -> 0x59bd88, b -> 0x59bd84
void fog_restore_level_default();                     // 0x486bc0
void char_fog_set(float near_dist, float far_dist);   // 0x481e00
void char_fog_disable();                              // 0x481e40
// Falling snow (decomp46): snow_start / snow_stop (the lenele3 levels and IonaExt02 call them from their
// hooks) moved with the rest of the snow unit to engine/rendering/unnamed_snow_0x4946a0.h (decomp54),
// included here so the callers keep compiling.
#include "engine/rendering/unnamed_snow_0x4946a0.h"
// Door objects within reach of the character (unit not known; next to the AI code).
bool door_in_use_range(Character *c, lobj *door);             // 0x476a30
// decomp35 (gametarget.cpp). (lobj, fraction) door_is_open of the door of a door object.
bool lobj_door_is_open(lobj *l, float fraction);              // 0x4769f0
bool lobj_is_door(lobj *l);           // 0x477310 flag 0x100 and a door whose +4 is >= 0
// The hover tints of levelrender_draw_list (unit not known): one object (hover_tint_obj 0x22fa6a4) and
// one level object (hover_tint_char 0x22f8d0c: the sum.symbols name says character, but the draw list
// compares it with level objects, D1503). (target, r, g, b).
void hover_tint_set_object(obj *o, float r, float g, float b);     // 0x4820b0
void hover_tint_set_character(lobj *l, float r, float g, float b); // 0x4820e0
float fog_get_far_current();         // 0x486c20
// decomp52 (the cutscene file): the fog distances (near, far: [esp+4] -> fog_near_distance 0x59bd80,
// [esp+8] -> fog_far_distance 0x59bd7c, <= 0 takes the level default), the forced visibility list of a
// cutscene $VisSet (0x22ea544, 16 max) and the level fade reset.
void fog_set_distances(float near_dist, float far_dist);   // 0x486b10
void level_vis_force_begin();         // 0x487150
void level_vis_force_reset();         // 0x487190
void level_vis_force_add(const char *name);   // 0x4871b0
void level_vis_force_end();           // 0x4871e0
void levelrender_fade_reset();        // 0x482070
// Zeroes the six overhead label slots (0x22f9560; unit not known). interface_set_screen calls it before
// the automap, spells and skills panels open (decomp48).
void overhead_label_slots_clear();   // 0x4872b0
// (bitmap, pos, yaw degrees, width, height): the target decal (unit not known).
void place_ground_decal(int bm, vector3 *pos, float yaw, float w, float h);   // 0x487800
// (ray, hit, iterator (NULL: from 0), flag (NULL), skip lists): ray against the level objects of the draw
// lists; continues from *iter, returns hit->num_hits. hit->face is the level object index, hit->_unk28
// the s3d_face hit (world_pick_lobj).
// Per level object render records (12 bytes; type name ours): +4 bits 4-11 all set = drawn opaque
// (world_pick_lobj).
struct lobj_render_state {
    int _unk00;
    unsigned int flags;
    int _unk08;
};
extern lobj_render_state *levelrender_lobj_states;   // 0x22f9d7c
extern bool levelrender_draw_chain_marker;   // 0x22fb6d4 set by chain_attack_select

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
extern int levelrender_d3d_vertex_buffer_pos; // 0x240d9e8 next free vertex; reset by gr_d3d_init_device / gr_d3d_flip (name ours, decomp36)
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

// ---- decomp42 (gameplay.cpp's calls; units not known) ---------------------------------------------------
void door_find_floor_point(lobj *door);               // 0x476ab0 (level_restore_doors)
bool hover_tint_active();                             // 0x482090 (gameplay_update_cursor)
void level_set_current_layer(int layer);              // 0x482110 level_current_layer (0x59bd90)
void levelrender_level_load();                        // 0x4865a0
void levelrender_level_unload();                      // 0x486ab0
bool levelrender_get_flag_22fb6f7();                  // 0x486be0 (game_render_frame's fog text)
float fog_get_near_current();                          // 0x486bf0
int  level_vis_load(const char *level_filename);      // 0x486c60 nonzero = failure
void move_target_set_bitmap(int bm);                  // 0x487750 move_target_bm (0x59c124)
void move_target_draw_decal();                        // 0x487760

#endif
