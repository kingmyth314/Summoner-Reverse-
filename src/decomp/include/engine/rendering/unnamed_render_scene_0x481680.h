// engine/rendering/unnamed_render_scene_0x481680.h - the level frame: fog, layers and visibility sets,
// draw-list building and drawing, overhead labels and health bars, lamp-glow visibility, detail levels;
// the unit 0x481680-0x48743f (source game/unnamed_render_scene_0x481680.cpp, decomp63). File and directory
// names are placeholders: no path string, no strings.tbl block (levelrender.cpp, whose path literal is at
// 0x59c524, starts at 0x48db30 after multichat.cpp and the fx unit). Not original source.
// The declarations of this unit that were in levelrender.h before decomp63 stay there (fog_*, level_vis_*,
// hover_tint_*, levelrender_render_scene, ...); this header adds the rest. Included at the end of
// levelrender.h.
#ifndef ENGINE_RENDERING_UNNAMED_RENDER_SCENE_0X481680_H
#define ENGINE_RENDERING_UNNAMED_RENDER_SCENE_0X481680_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/console.h"
#include "engine/s3d/s3d.h"
#include "engine/rendering/levelrender.h"

class Character;
class item_obj;
class container;
class effect_object;
struct character_speech_record;
class TextFile;

#define LEVEL_NUM_LAYERS       20     // the per-layer tables (fog, camera, snow, vis sets)
#define LEVEL_MAX_VIS_SETS     0x200  // level_vis_set_records / level_vis_set_lists
#define LEVEL_MAX_FORCED_VIS   16     // level_vis_force_list (cutscene $VisSet)
#define LEVELRENDER_MAX_ATTACH 2000   // levelrender_attach_list
#define LEVELRENDER_MAX_SORTED 500    // levelrender_sort_list / levelrender_opaque_list
#define LEVELRENDER_POOL_SIZE  0xc800 // levelrender_entry_pool
#define LEVELRENDER_MAX_FLOAT_TEXTS 16
#define LEVELRENDER_MAX_LEVEL_LIGHTS 24
#define MAX_DETAIL_LEVELS      41     // detail_levels.tbl records

// The visibility of one layer (12 bytes, level_layer_vis[20]; names ours): the vis sets drawn while the
// player stands on it, a list for the low and one for the high fog quality (the "HIGH" variants of the
// sets; Summoner_options.fog_quality 0 takes the first).
struct level_layer_vis_rec {
    int count;                        // +0x00
    s3d_vis_set **sets;               // +0x04 into level_vis_set_lists
    s3d_vis_set **sets_high;          // +0x08 into level_vis_set_records
};
LAYOUT_CHECK(level_layer_vis_rec, sizeof(level_layer_vis_rec) == 0xc);

// One detail_levels.tbl record (0x28 bytes; names ours): a level name, its fog multiplier and the flags
// detail_apply_level_settings applies.
struct detail_level {
    char name[0x20];                  // +0x00 ParseString(.., 0x1e, '"', '"'); matched against the level file
    float fog_multiplier;             // +0x20 Gr_level_fog_multiplier (halfway to 1.0 at fog quality 1)
    bool unlock_medium;               // +0x24 "+UnlockMedium": the high camera at fog quality 1 (and 2)
    bool unlock_high;                 // +0x25 "+UnlockHigh" (or "+UnlockMedium"): ... at fog quality 2
    bool small_mesh_culling;          // +0x26 false with "+NoSmallMeshCulling"
    ubyte _pad27;
};
LAYOUT_CHECK(detail_level, sizeof(detail_level) == 0x28);

// The lamp glow of a level object and the object that hid it last (4 bytes per level object; names ours).
struct lobj_glow_ref {
    short glow;                       // +0 lamp_glow_add index (objects with flag 8 only)
    short occluder;                   // +2 level object index, -1 none
};

// Where a speech line was drawn (12 bytes, 2 sides x 3 rows; levelrender_draw_name_labels puts the
// speaker's name there). The type is a vector with an empty constructor (one of the three empty
// initialisers at the front of the unit, unsure which).
class overhead_label_pos : public vector3 {
public:
    overhead_label_pos() {}
};

// A scope object whose constructor and destructor are empty inline functions: levelrender_render_scene,
// levelrender_fade_occluders, levelrender_draw_list and lamp_glows_draw open EH frames whose only unwind
// actions are the folded empty destructor 0x434650 (empty_hook_434650), with no constructor code. A
// profiling macro of the original source is the likely reason; the class and its name are ours.
class render_scope {
public:
    render_scope() {}
    ~render_scope() {}
};

// ---- .data (0x59bd70-0x59bdab) ------------------------------------------------------------------------
extern int detail_level_entry_index;      // 0x59bd70 -1 none
// path_internal 0x59bd74 (= -1) is declared in summoner/summoner.h.
extern bool levelrender_show_float_text;  // 0x59bd78 1 (name ours; levelrender_draw_float_text)
extern bool Render_level;                 // 0x59bd79 1 console "render_level" (nothing else reads it)
extern bool Render_characters;            // 0x59bd7a 1 console "render_characters" (nothing else reads it)
extern bool Render_effects;               // 0x59bd7b 1 console "render_effects" (nothing else reads it)
extern float fog_far_distance;            // 0x59bd7c 50.0 gr_set_fog's far distance and the far clip
extern float fog_near_distance;           // 0x59bd80 35.0 gr_set_fog's near distance
extern float fog_level_far_distance;      // 0x59bd84 50.0
extern float fog_level_near_distance;     // 0x59bd88 35.0
extern int levelrender_59bd8c;            // 0x59bd8c 1, never accessed (name ours)
extern int level_current_layer;           // 0x59bd90 -1
extern int level_rendered_layer;          // 0x59bd94 -1
extern bool levelrender_mark_far_living;  // 0x59bd98 1 (name ours; levelrender_build_draw_lists)
extern float Main_character_lod_dist;     // 0x59bd9c 9.0 console "main_lod_dist"
extern float Character_lod_dist;          // 0x59bda0 27.0 console "lod_dist"
extern int levelrender_fade_alpha;        // 0x59bda4 0xff
extern bool Cull_ground_too;              // 0x59bda8 1 console "Cull_ground_too"

// ---- .bss (0x22ea498-0x22fb71b) ------------------------------------------------------------------------
extern int levelrender_22ea498;           // 0x22ea498 zeroed every frame, never read (name ours)
extern float hover_tint_obj_r;            // 0x22ea49c
extern float hover_tint_obj_g;            // 0x22faec8
extern float hover_tint_obj_b;            // 0x22faee0
extern obj *hover_tint_obj;               // 0x22fa6a4
extern lobj *hover_tint_char;             // 0x22f8d0c (a level object, D1503)
extern float hover_tint_char_r;           // 0x22fa69c
extern float hover_tint_char_g;           // 0x22f8d10
extern float hover_tint_char_b;           // 0x22f8cec
extern int level_layer_vis_flags[LEVEL_NUM_LAYERS];      // 0x22ea4a4 0 low camera, 1 "$high_camera"
extern float level_layer_fog_near[LEVEL_NUM_LAYERS];     // 0x22ea4f4 fog_set_layer's third argument
extern float level_layer_fog_far[LEVEL_NUM_LAYERS];      // 0x22fae78 fog_set_layer's second argument
extern ubyte level_layer_fog_r[LEVEL_NUM_LAYERS];        // 0x22fb6a4
extern ubyte level_layer_fog_g[LEVEL_NUM_LAYERS];        // 0x22fb690
extern ubyte level_layer_fog_b[LEVEL_NUM_LAYERS];        // 0x22fb67c
extern bool level_layer_snow[LEVEL_NUM_LAYERS];          // 0x22faffc
extern level_layer_vis_rec level_layer_vis[LEVEL_NUM_LAYERS];   // 0x22faee8
extern s3d_vis_set *level_vis_force_list[LEVEL_MAX_FORCED_VIS]; // 0x22ea544
extern int level_vis_force_count;         // 0x22faedc
extern bool level_vis_forced;             // 0x22fb715
extern s3d_vis_set *level_vis_set_lists[LEVEL_MAX_VIS_SETS];    // 0x22f9e9c the sets as named (name ours)
extern s3d_vis_set *level_vis_set_records[LEVEL_MAX_VIS_SETS];  // 0x22f957c their "HIGH" variants
extern int level_num_vis_sets;            // 0x22fb700
extern bool level_vis_dirty;              // 0x22fb708
extern int level_vis_cache_key[2];        // 0x22fb70c the vis list and count levelrender_add_effects saw
extern levelrender_entry *levelrender_hover_target;   // 0x22f8d14 the entry of world_hover_object
extern attached_light *levelrender_level_lights[LEVELRENDER_MAX_LEVEL_LIGHTS];   // 0x22f8d18
extern int levelrender_level_light_handles[LEVELRENDER_MAX_LEVEL_LIGHTS];        // 0x22f9e28 (name ours)
extern int levelrender_num_level_lights;  // 0x22fb704
extern int levelrender_num_float_texts;   // 0x22ec4d4
extern levelrender_entry *levelrender_float_texts[LEVELRENDER_MAX_FLOAT_TEXTS];  // 0x22f9d8c
extern ubyte levelrender_entry_pool[LEVELRENDER_POOL_SIZE];   // 0x22ec4e4
extern ubyte *levelrender_entry_pool_cursor;                  // 0x22f9e88
extern bool overhead_label_slot_used[2][3];                   // 0x22f9558 [side][row]
extern character_speech_record *overhead_label_slot_owner[2][3];   // 0x22f9560
extern overhead_label_pos overhead_label_positions[2][3];     // 0x22f9de0 (name ours)
extern bool lamp_glows_pending;           // 0x22f9578 a lamp object is in the draw lists (name ours)
extern lobj_glow_ref *levelrender_lobj_glow_map;   // 0x22fa6a0
extern int levelrender_lobj_state_bytes;  // 0x22faecc
extern vector3 levelrender_eye_pos;       // 0x22f9e90
extern matrix levelrender_eye_orient;     // 0x22fafd8
extern detail_level detail_levels[MAX_DETAIL_LEVELS];   // 0x22fb010
extern int num_detail_levels;             // 0x22fb6c0
extern int fog_color_r;                   // 0x22fb6d8
extern int fog_color_g;                   // 0x22fb6dc
extern int fog_color_b;                   // 0x22fb6e0
extern float char_fog_near;               // 0x22fb6e4
extern float char_fog_far;                // 0x22fb6e8
extern float char_fog_near_sq;            // 0x22fb6ec
extern float char_fog_far_sq;             // 0x22fb6f0
extern bool char_fog_enabled;             // 0x22fb6f4
extern bool fog_level_default_set;        // 0x22fb6f5
extern bool levelrender_22fb6f6;          // 0x22fb6f6 cleared by levelrender_level_load, never read (ours)
extern bool levelrender_flag_22fb6f7;     // 0x22fb6f7 cleared every frame, never set (name ours)
extern bool fog_fading;                   // 0x22fb6f8 never set: the fog fade code is dead
extern float fog_fade_amount;             // 0x22fb6fc
extern bool levelrender_no_lighting;      // 0x22fb714 never set (name ours; no level lights, no culling)
extern bool Cull_ground;                  // 0x22fb716 console "Cull_ground"
extern bool levelrender_fade_in;          // 0x22fb717
extern bool levelrender_layer_fog_applied;   // 0x22fb718
extern console_command ccmd_lod_dist;     // 0x22ec4d8
extern console_command ccmd_main_lod_dist;   // 0x22f8d78
extern console_command ccmd_render_level; // 0x22f8cf0
extern console_command ccmd_render_characters;   // 0x22faed0
extern console_command ccmd_render_effects;  // 0x22f8d00
extern console_command ccmd_set_fog;      // 0x22f9d80
extern console_command ccmd_Cull_ground;  // 0x22ec4c8
extern console_command ccmd_Cull_ground_too; // 0x22f9dd0

// ---- functions -----------------------------------------------------------------------------------------
void cmd_lod_dist();                      // 0x481710
void cmd_main_lod_dist();                 // 0x4817d0
void cmd_render_level();                  // 0x481890
void cmd_render_characters();             // 0x481980
void cmd_render_effects();                // 0x481a70
void cmd_set_fog();                       // 0x481b60
void cmd_Cull_ground();                   // 0x481ea0
void cmd_Cull_ground_too();               // 0x481f90
// (character, bitmap): the bitmap above the "BDBN-Head" / "Head" tag (party members, Ulsadana, or a
// visible character within 50 m and 4 m of height of cur_character).
void character_draw_overhead_bitmap(Character *c, int bm);   // 0x481c40
void levelrender_fade_out_start();        // 0x482050
void levelrender_fog_enable();            // 0x482850
// (vis set list, count): the alpha fades of the camera occluders and their "$seethru" sets.
void levelrender_fade_occluders(s3d_vis_set **sets, int count);   // 0x4829e0
void levelrender_fog_disable();           // 0x482d40
void levelrender_setup_level_lights();    // 0x482d70
void levelrender_build_draw_lists();      // 0x482e20
void levelrender_add_character(Character *c);   // 0x483260
// (character, item type, prop tag): the item's mesh at the bone of that tag (char_info +0x168[tag]).
void levelrender_add_bone_attachment(Character *c, ItemType *item, int tag);   // 0x4833b0
// (item, character, bone position, bone orientation, alpha (not used), light level).
void levelrender_push_attachment(ItemType *item, Character *c, const vector3 *pos, const matrix *orient,
                                 ubyte alpha, float light);   // 0x483430 (D4307)
void levelrender_add_items(item_obj *it);          // 0x483570 (one object; the name is sum.symbols')
void levelrender_add_containers(container *ct);    // 0x4836f0 (one object)
void levelrender_add_lobjs(s3d_vis_set **sets, int count);   // 0x483850
void levelrender_add_lobj(lobj *l, int index);      // 0x4838f0
void levelrender_add_effects(s3d_vis_set **sets, int count); // 0x483ba0
void levelrender_effect_apply_vis(effect_object *fx, s3d_vis_set **sets, int count);   // 0x483f90
// (position in the effect frame, cull radius, sort radius, effect, kind, index, instance record, cull):
// false when nothing was added.
bool levelrender_add_effect_entry(vector3 pos, float cull_radius, float sort_radius, effect_object *fx,
                                  int kind, int index, void *inst, bool cull);   // 0x4841e0
void lamp_glows_draw();                   // 0x484380
void lamp_glow_update_lobjs();            // 0x4843e0
void lamp_glow_update_effects();          // 0x484600
void levelrender_sort_back_to_front(int count, levelrender_entry **list);   // 0x484830
void levelrender_draw_health_bars(levelrender_entry *e);   // 0x4848c0
void overhead_label_slots_expire();       // 0x484aa0
void levelrender_draw_speech_text(character_speech_record *s);   // 0x484ae0
// (screen x, screen y, label height, in/out side, in/out row): moves the label to a free slot near
// the one its screen position gives.
void overhead_label_place(int x, int y, int h, int *side, int *row);   // 0x484e30
void levelrender_draw_name_labels();      // 0x4850a0
void levelrender_draw_float_text(levelrender_entry *e);   // 0x485380
void levelrender_draw_speaker_icons();    // 0x485810
// (count, list, sorted): sorted is not read (the alpha comes from the records).
void levelrender_draw_list(int count, levelrender_entry **list, bool sorted);   // 0x4858f0
void levelrender_rank_light(lobj *l, attached_light *light, unsigned int *count, float *weights,
                            attached_light **lights);   // 0x4863b0
unsigned int levelrender_lobj_collect_projections(int index);   // 0x486520 the projector count
void detail_levels_parse_table();         // 0x4868f0
void levelrender_apply_lobj_detail_flag();// 0x486a40
void level_vis_parse(TextFile *tf);       // 0x486d40
void level_vis_reset();                   // 0x487030
void level_vis_invalidate();              // 0x487170
bool fog_layer_snow_allowed(int layer);   // 0x487210
void fx_draw_flagged_particles();         // 0x487230
void detail_apply_level_settings();       // 0x487370

#endif
