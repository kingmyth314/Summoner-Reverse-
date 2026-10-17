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
#include "vsdk/geom/vfx.h"                 // vfx_node_ref (decomp63)

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
    float _unk18;                     // +0x18 sphere radius: below 0.0001 a thin ray (lobj_ray_test_geometry; decomp71)
    int flags;                        // +0x1C bit 1 tests only the face _unk2c (lobj_raycast); lobj_raycast_all:
                                      //       bit 0 stop at the first hit, bit 2 stop at a hit nearer than stop_t
    unsigned int require_flags;       // +0x20 lobj flags that must all be set
    unsigned int exclude_flags;       // +0x24 lobj flags that must be clear
    union {                           // +0x28
        int _unk28;
        float stop_t;                 //       decomp71: lobj_raycast_all compares hit.t with it (fcomp at 0x4af300)
    };
    union {                           // +0x2C
        int _unk2c;
        s3d_face *face;               //       decomp71: the one face tested with flag 2 (living_entity.cpp stores
                                      //       &mesh->faces[i] cast to int)
    };
    union {                           // +0x30
        int _unk30;
        float min_radius;             //       decomp57: lobj_raycast_draw_lists skips objects whose mesh radius
                                      //       (geometry +0x30) is below it (fcomp at 0x4a4c36)
    };
    vector3 local_start;              // +0x34 written by lobj_raycast (ray in the object frame)
    vector3 local_dir;                // +0x40
};
LAYOUT_CHECK(lobj_ray, sizeof(lobj_ray) == 0x4c);

// The hit record (0x2c bytes; names ours). lobj_raycast clears +0x00, +0x20, +0x28 when its flag is 1.
struct lobj_hit {
    int num_hits;                     // +0x00
    float t;                          // +0x04 hit distance as a fraction of ray.dir
    vector3 pos;                      // +0x08 the hit point (character_probe_lobj_ground; decomp37, was _unk08[6])
    vector3 normal;                   // +0x14 the face plane's normal towards the ray (decomp71; was int _unk14[3])
    int _unk20;                       // +0x20 1 when lobj_sphere_test_faces hit an edge or vertex (decomp71)
    int face;                         // +0x24 the level object index (world_pick_lobj, level_scripts.cpp's
                                      //        lobj_from_index(hit.face); character_probe_lobj_ground reads it
                                      //        as a word); copied to item_obj +0xa8
    union {
        int _unk28;                   // +0x28 the s3d_face hit (world_pick_lobj: minus mesh->faces = face index)
        s3d_face *face_ptr;           //       the same as a pointer (geometry face record, 0x14 bytes; decomp37)
    };
};
LAYOUT_CHECK(lobj_hit, sizeof(lobj_hit) == 0x2c);

// Level object ray tests (unit 0x4ae890-0x4af335, file not known; game/unnamed_lobj_raycast_0x4ae890.cpp,
// decomp71).
void lobj_raycast(int lobj_index, lobj_ray *ray, lobj_hit *hit, bool clear);   // 0x4ae890
void lobj_raycast_all(lobj_ray *ray, lobj_hit *hit);                           // 0x4af2c0
// decomp71: the geometry tests (face, geometry, lobj index, ray in the object frame, hit).
void lobj_ray_test_geometry(s3d_face *face, s3d_geometry *g, int lobj_index, lobj_ray *ray, lobj_hit *hit);  // 0x4aea00
void lobj_ray_test_faces(s3d_face *face, s3d_geometry *g, int lobj_index, lobj_ray *ray, lobj_hit *hit);     // 0x4aea50
void lobj_sphere_test_faces(s3d_face *face, s3d_geometry *g, int lobj_index, lobj_ray *ray, lobj_hit *hit);  // 0x4aec20
// (the geometry's node block, its first node, geometry, lobj index, ray, hit).
void lobj_raycast_bsp(s3d_geom_block *block, void *root, s3d_geometry *g, int lobj_index, lobj_ray *ray,
                      lobj_hit *hit);                                                                       // 0x4aeef0
// The face and object of the last hit of the two face tests (written, never read; names ours).
extern s3d_face *lobj_last_hit_face;                 // 0x247e81c
extern int lobj_last_hit_index;                      // 0x247e820
// decomp37 (living_entity.cpp):
// decomp57 (D3710): returns the hit count; `blocked` is the list stage byte (0 attachments, 1 opaque,
// 2 sorted; written through a char pointer, it takes the value 2), the last argument a bool (when true
// and no attachment entries exist, nothing is tested). Both pointers may be NULL.
int  lobj_raycast_draw_lists(lobj_ray *ray, lobj_hit *hit, int *lobj_index, bool *blocked, bool attach_only);   // 0x4a4bb0
int  lobj_get_count();                // 0x477770 num_lobjs (decomp57)
// The draw-list entries (levelrender_build_draw_lists / add_lobj / push_attachment; decomp57, completed in
// decomp63 from the render-scene unit 0x481680). Bump-allocated from levelrender_entry_pool every frame.
// type: 0 living entity, 1 level object, 2 attachment (prop or weapon on a bone), 3 effect node / emitter /
// trail, 4 item object, 5 container (the cases of levelrender_draw_list).
class Character;
class effect_object;
#define LRE_CHARACTER  0
#define LRE_LOBJ       1
#define LRE_ATTACHMENT 2
#define LRE_EFFECT     3
#define LRE_ITEM       4
#define LRE_CONTAINER  5
struct levelrender_entry {
    char type;                        // +0x00 LRE_*
    char _unk01;
    short lobj;                       // +0x02 level object index (type 1)
    union {
        int _unk04;
        float depth;                  // +0x04 eye-space depth (dot with the eye's fvec; decomp63)
    };
    float dist;                       // +0x08 sort key: depth minus the object's radius
    obj *object;                      // +0x0c types 0, 4, 5 (decomp63)
};
LAYOUT_CHECK(levelrender_entry, sizeof(levelrender_entry) == 0x10);
// A prop drawn at a bone (0x4c bytes; levelrender_push_attachment; names ours).
struct levelrender_attach_entry : levelrender_entry {
    Character *character;             // +0x10
    ItemType *item;                   // +0x14 its mesh (+0x10) is drawn
    float light;                      // +0x18 the character's light level (+0x780)
    vector3 pos;                      // +0x1c bone position
    matrix orient;                    // +0x28 bone orientation
};
LAYOUT_CHECK(levelrender_attach_entry, sizeof(levelrender_attach_entry) == 0x4c);
// An effect node, emitter or line trail (0x1c bytes; levelrender_add_effect_entry; names ours).
struct levelrender_effect_entry : levelrender_entry {
    effect_object *fx;                // +0x10
    vfx_node_ref ref;                 // +0x14 what vfx_instance::render_node draws
};
LAYOUT_CHECK(levelrender_effect_entry, sizeof(levelrender_effect_entry) == 0x1c);
extern levelrender_entry *levelrender_attach_list[];   // 0x22ea584
extern levelrender_entry *levelrender_sort_list[];     // 0x22f8d84
extern levelrender_entry *levelrender_opaque_list[];   // 0x22fa6a8
extern int levelrender_attach_count;  // 0x22fb6c4
extern int levelrender_opaque_count;  // 0x22fb6c8
extern int levelrender_sort_count;    // 0x22fb6cc
extern bool shadows_enabled;           // 0x59c120 (1)
extern int lamp_glow_bitmap;           // 0x59db70 (lamp_glow_load)
int  lobj_get_layer(int lobj_index);                  // 0x4775c0 the first layer byte
float char_fog_get_far();                             // 0x481e50 the +CharFog far distance, 0 when off (unit not known)
void door_update(lobj *door);         // 0x477080 (decomp58: unnamed_goals_doors_0x475180.cpp, as all door and lobj helpers here)
// A door record (0x8c bytes, sorted by lobj index at 0xa5a304; door_update reads both poses). Type and
// field names ours (D100). s3d_read_door reads +0x00..+0x43 and resets the rest (decomp28, D802).
struct door_info {
    int lobj_index;                   // +0x00
    short named_path;                 // +0x04 level named path the door takes its name from, -1 none
    short _unk06;                     // +0x06
    union {
        float _unk08;                 // +0x08
        float duration;               //       seconds a move takes (door_update: elapsed / (duration * 1000); decomp58)
    };
    vector3 closed_pos;               // +0x0c door_update reads +0x10 and +0x1c too
    quaternion closed_orient;         // +0x18
    vector3 open_pos;                 // +0x28
    quaternion open_orient;           // +0x34
    GameTimer timer;                  // +0x44 move timer (constructed by new door_info[n])
    float _unk48[3];                  // +0x48 the floor point (door_find_floor_point; door_in_use_range reads +0x4c)
    // decomp58: the door state is a bit field (door_start_move: `sete; shl; and; or` merges, the delay
    // field `shl 4; xor (state & 0xf)` and `shr 4` in door_update; names ours, D3804). s3d.cpp and the
    // savegame code keep the whole-word name.
    union {
        unsigned int state;           // +0x54 bits 0, 1 and 3 cleared on load; bit 3 the default locked
                                      //        state (savegame_two_stage_get, decomp27)
        struct {
            unsigned int moving : 1;        // bit 0 the move timer runs
            unsigned int opening : 1;       // bit 1 direction of the move
            unsigned int close_wait : 1;    // bit 2 waiting to close again (door_update)
            unsigned int locked_default : 1;// bit 3
            unsigned int close_delay : 28;  // bits 4.. auto-close delay in ms + 1, 0 none (door_start_move)
        };
    };
    vector3 &floor_point() { return *(vector3 *)_unk48; }   // (accessor ours; no code)
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
// The door commands' helpers (decomp27; decomp58: the goal unit 0x475180). door_find_by_name: 0 found, -1 not (the outputs
// are then not written).
int  door_find_by_name(const char *name, lobj **out_lobj, door_info **out_door);   // 0x477280
// decomp58: the flags are bools (passed through to door_start_move, which tests bytes; were int).
bool door_set_open(lobj *l, float close_delay, bool force, bool sound);   // 0x476e00 (cmd_door_open: -1.0, 1, 1)
void door_set_closed(lobj *l);        // 0x476e80 (return unused)
// false when the door does not move (locked); object_try_unlock_or_open tests AL (D1504).
bool door_toggle_state(lobj *l, bool force);   // 0x476ee0 (cmd_door_toggle passes 1, gametarget 0; decomp58: bool, was int)
lobj *lobj_from_index(int index);     // 0x477780
// Level-script door and object helpers (decomp34, D1403; decomp58: the goal unit 0x475180).
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
// fog_set_layer stores a into level_layer_fog_near, b into level_layer_fog_far (sum.symbols names).
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
// Door objects within reach of the character (decomp58: the goal unit 0x475180).
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
// (world_pick_lobj). decomp63 (levelrender_level_load / levelrender_fade_occluders): +0 the "$seethru"
// vis set the object belongs to (its members fade with it), +4 the fade bits below, +8 the fade timer.
// The bits are written with explicit masks: the code's and / or patterns are not those of bit fields.
#define LRS_FADE_HIDDEN   0x001       // bit 0: faded out (state 1), with bit 1 fading out (state 3)
#define LRS_FADE_MOVING   0x002       // bit 1: fading in (state 2) or out (state 3)
#define LRS_OCCLUDING     0x004       // between the camera and the target this frame
#define LRS_WAS_OCCLUDING 0x008       // ... the frame before
#define LRS_ALPHA_SHIFT   4           // bits 4-11: the alpha the back ends draw with (0xff opaque)
#define LRS_ALPHA_MASK    0xff0
struct lobj_render_state {
    union {
        int _unk00;
        s3d_vis_set *seethru;         // +0x00 NULL none
    };
    unsigned int flags;               // +0x04 LRS_*
    GameTimer timer;                  // +0x08 (was int _unk08)
};
LAYOUT_CHECK(lobj_render_state, sizeof(lobj_render_state) == 0xc);
extern lobj_render_state *levelrender_lobj_states;   // 0x22f9d7c
extern bool levelrender_draw_chain_marker;   // 0x22fb6d4 set by chain_attack_select

void levelrender_render_scene();      // 0x482200
void options_apply_detail();          // 0x4872d0 (detail settings; name from sum.symbols)
void levelrender_clear_draw_lists();  // 0x486b00

// (lobj, face, point, out r, g, b): writes 0xff to the three output bytes (unit not known; decomp23).
void lighting_sample_stub(int lobj, int face, const float *point, ubyte *r, ubyte *g, ubyte *b);  // 0x4af320
// Shadows and level effects. shadows_parse_table (0x4874c0) is in the shadow-table unit after
// options_apply_detail; shadows_init (0x48a060) and fx_shutdown (0x48a0e0) are after the experience table, in
// the effect-object unit (unnamed_fx_0x489c40.cpp). Called by summoner.cpp's GameInit / game_cleanup.
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
// decomp63: the three back ends levelrender_draw_list calls by gr_screen.mode (0x66 / 0x67 / 0x68) with
// (level object, alpha 0..255, light count, the ranked lights); written by decomp76 (levelrender.cpp).
class attached_light;
void levelrender_draw_list_glide(lobj *l, int alpha, unsigned int num_lights, attached_light **lights);    // 0x48e6c0
void levelrender_draw_list_d3d(lobj *l, int alpha, unsigned int num_lights, attached_light **lights);      // 0x48fdb0
void levelrender_draw_list_opengl(lobj *l, int alpha, unsigned int num_lights, attached_light **lights);   // 0x491500
// decomp63: levelrender.cpp .bss bytes the render-scene unit reads (names ours). The first is never set
// to 1 by any instruction; the second guards a one-time loop over levelrender_lights[16] (+0x55 = 0).
extern bool levelrender_lights_dirty;         // 0x243e038
extern bool levelrender_lights_initialized;   // 0x243e039
extern vector3 levelrender_wave_vertices[];   // 0x23ec658 (levelrender.cpp)
extern attached_light levelrender_lights[16];  // 0x23df188 (levelrender.cpp, name ours)
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

// Level object helpers (decomp58: the goal unit 0x475180-0x4778ab, game/unnamed_goals_doors_0x475180.cpp,
// file name not known).
s3d_textured_record *lobj_find_textured_record(int lobj_index);   // 0x477450 bsearch of level +0x70
int  name_ptr_stricmp_compare(const void *a, const void *b);      // 0x4768a0 qsort / bsearch comparator
// bsearch comparator of two records that start with an int (the lobj index): *a - *b. No symbol (the
// gap 0x476890-0x47689e before name_ptr_stricmp_compare); name ours (decomp58).
int  lobj_index_compare(const void *a, const void *b);            // 0x476890
// (lobj, auto-close delay in seconds (< 0 none), ignore the lock, play the sound): false when locked.
// Flips the saved two-stage state and starts the move timer.
bool door_start_move(lobj *l, float close_delay, bool force, bool sound);   // 0x476f30
s3d_vis_set *level_vis_record_find(const char *name);  // 0x477540 (level_vis_parse, level_vis_force_add)
bool lobj_index_out_of_range(int index);              // 0x4776b0 index >= num_lobjs
void lobj_get_bounding_sphere(int index, vector3 *centre, float *radius);   // 0x477720
// The fog defaults of the two camera presets (unit not known; used only when the layer has no fog
// distance of its own).
void fog_set_high_camera_default();   // 0x487080 (camera_view_high)
void fog_set_low_camera_default();    // 0x4870b0 (camera_view_low)
// Ken Perlin's reference noise over a vector (the vfx code calls it too). decomp69: the declaration
// (0x5470e0) moved to the noise unit's header.
#include "vsdk/math/unnamed_perlin_0x546d00.h"

// ---- decomp42 (gameplay.cpp's calls; units not known) ---------------------------------------------------
void door_find_floor_point(lobj *door);               // 0x476ab0 (level_restore_doors)
// decomp63 (D4300): int, was bool (mov eax, 1 / xor eax, eax at 0x4820a5 / 0x4820a2).
bool hover_tint_active();                             // 0x482090 (gameplay_update_cursor; bool, D6700)
void level_set_current_layer(int layer);              // 0x482110 level_current_layer (0x59bd90)
// decomp63 (D4300): returns 0 (xor eax, eax at 0x4868dd); was void. gameplay.cpp ignores it.
int  levelrender_level_load();                        // 0x4865a0
void levelrender_level_unload();                      // 0x486ab0
bool levelrender_get_flag_22fb6f7();                  // 0x486be0 (game_render_frame's fog text)
float fog_get_near_current();                          // 0x486bf0
int  level_vis_load(const char *level_name);      // 0x486c60 nonzero = failure
void move_target_set_bitmap(int bm);                  // 0x487750 move_target_bm (0x59c124)
void move_target_draw_decal();                        // 0x487760

// decomp59 (Murod leap, Luminar teleport): the character whose tint levelrender_draw_list overrides with
// these components (levelrender.cpp's .bss; names from sum.symbols).
extern Character *levelrender_fx_char;                // 0x22fb6d0
extern float levelrender_fx_char_tint_r;              // 0x22fb678
extern float levelrender_fx_char_tint_g;              // 0x22ea4a0
extern float levelrender_fx_char_tint_b;              // 0x22f8ce8

// decomp63: the render-scene unit 0x481680-0x48743f and the shadow-table unit 0x487440-0x48789f (both
// file names placeholders).
#include "engine/rendering/unnamed_render_scene_0x481680.h"
#include "engine/rendering/unnamed_shadow_table_0x487440.h"

#endif
