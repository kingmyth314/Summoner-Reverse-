// unnamed_render_scene_0x481680.cpp - Sum.exe v1.40, code 0x481680-0x48743f: the level frame (fog, the
// layer visibility sets, building, sorting and drawing the draw lists, level-object fades and lamp-glow
// visibility, overhead speech / name labels, floating texts and health bars, detail_levels.tbl, the .vis
// file) and its console commands lod_dist, main_lod_dist, render_level, render_characters,
// render_effects, set_fog, Cull_ground and Cull_ground_too.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"). levelrender.cpp (path literal 0x59c524) is a different
// object that starts at 0x48db30. Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs, llvm-objdump). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 /Ob1. Confidence per function: exact (control flow, calls, constants and globals match), likely (same,
// but one detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range (decomp63, NOTES.md): static initialiser entries 239-249 at its front (0x481680, after
// multiendgame.cpp's last RET 0x481673, entry 238), last function detail_apply_level_settings (its RET
// at 0x487432). The shadow-table unit starts at 0x487440: its .data starts with the variable shadows_enabled
// (0x59c120 = 1) right after this unit's last literal ("$vis_set", 0x59c114), and its .bss
// (shadow_entries 0x22fb720) after this unit's last byte (0x22fb718).
// .data 0x59bd70-0x59c11f: the variables 0x59bd70-0x59bdab, then the literals ("lod_dist" 0x59bdac ..
// "$vis_set" 0x59c114). The bool-command texts (0x588c34-0x588c50), the float-command texts
// (0x5948ec / 0x5948f8), "Head" 0x59935c, "BDBN-Head" 0x599a10, "$vis" 0x593aa8, "\"" 0x593ab0, "high"
// 0x5963f8, ":" 0x5986ac, "%d" 0x589820, "%d%%" 0x592a10, "Gold" 0x58d4a8, "#End" are pooled copies in
// other units (/Gf).
// .bss 0x22ea498-0x22fb71b, between multiendgame.cpp's mp_net_handle_map (ends 0x22ea493) and the
// shadow-table unit. The order of the .bss objects below is the address order; VC6 does not keep the
// definition order there (the hover tint components are spread over the block), so it is no evidence.
// .rdata: this unit's own float constants are 0x581a80-0x581aa8 (2500, -256, 256, 24, 900, 50625, 1600,
// 5625, 1200, 32, 127); the others are shared (__real@ pooling).
//
// The draw lists: every frame levelrender_render_scene resets the 0xc800-byte entry pool and fills three
// lists: attach (attachments, items, containers and level objects without alpha), opaque (living entities
// with an opaque model, level objects whose faces have alpha (lobj flag 0x10), the nodes of opaque effects) and sort
// (everything translucent), the last sorted back to front by depth minus radius.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/geom/effect_mem.h"
#include "engine/s3d/s3d.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/camera/camera.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "engine/rendering/unnamed_decals_0x4bbe80.h"
#include "engine/rendering/unnamed_projectors_0x4c0250.h"
#include "engine/rendering/unnamed_lamp_glow_0x4c0a40.h"
#include "engine/rendering/unnamed_shadows_0x4a37d0.h"
#include "levelscripts/conversation.h"
#include "levelscripts/level_scripts.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "summoner/multi/multi.h"

#define LR_PI_OVER_180 0.017453f      // not used here (0x57ef38 is place_ground_decal's)

// ---- .data (0x59bd70-0x59bdab) ------------------------------------------------------------------------
int detail_level_entry_index = -1;        // 0x59bd70
int path_internal = -1;                   // 0x59bd74 (setup_data_paths stores the data\internal id)
bool levelrender_show_float_text = true;  // 0x59bd78 (name ours)
bool Render_level = true;                 // 0x59bd79
bool Render_characters = true;            // 0x59bd7a
bool Render_effects = true;               // 0x59bd7b
float fog_far_distance = 50.0f;           // 0x59bd7c
float fog_near_distance = 35.0f;          // 0x59bd80
float fog_level_far_distance = 50.0f;     // 0x59bd84
float fog_level_near_distance = 35.0f;    // 0x59bd88
int levelrender_59bd8c = 1;               // 0x59bd8c (never accessed; name ours)
int level_current_layer = -1;             // 0x59bd90
int level_rendered_layer = -1;            // 0x59bd94
bool levelrender_mark_far_living = true;  // 0x59bd98 (name ours)
float Main_character_lod_dist = 9.0f;     // 0x59bd9c
float Character_lod_dist = 27.0f;         // 0x59bda0
int levelrender_fade_alpha = 0xff;        // 0x59bda4
bool Cull_ground_too = true;              // 0x59bda8

// ---- .bss (0x22ea498-0x22fb71b) -----------------------------------------------------------------------
// 0x481680 init_empty_481690_thunk: jmp init_empty_481690 (static initialiser entry 239). Confidence: exact.
// 0x481690 init_empty_481690: an empty dynamic initialiser (bare RET). Confidence: exact.
// 0x4816a0 init_empty_4816b0_thunk: jmp init_empty_4816b0 (entry 240). Confidence: exact.
// 0x4816b0 init_empty_4816b0: an empty dynamic initialiser. Confidence: exact.
// 0x4816c0 init_empty_4816d0_thunk: jmp init_empty_4816d0 (entry 241). Confidence: exact.
// 0x4816d0 init_empty_4816d0: an empty dynamic initialiser. Confidence: exact.
// The three objects with empty inline constructors are the unit's vector-typed objects below; which
// entry belongs to which object is not known (unsure).
overhead_label_pos overhead_label_positions[2][3];   // 0x22f9de0 (name ours)
vector3 levelrender_eye_pos;              // 0x22f9e90
matrix levelrender_eye_orient;            // 0x22fafd8

int levelrender_22ea498;                  // 0x22ea498 (name ours)
float hover_tint_obj_r;                   // 0x22ea49c
float levelrender_fx_char_tint_g;         // 0x22ea4a0
int level_layer_vis_flags[LEVEL_NUM_LAYERS];         // 0x22ea4a4
float level_layer_fog_near[LEVEL_NUM_LAYERS];        // 0x22ea4f4
s3d_vis_set *level_vis_force_list[LEVEL_MAX_FORCED_VIS];   // 0x22ea544
levelrender_entry *levelrender_attach_list[LEVELRENDER_MAX_ATTACH];   // 0x22ea584
int levelrender_22ec4c4;                  // 0x22ec4c4 never accessed (name ours)
int levelrender_num_float_texts;          // 0x22ec4d4
ubyte levelrender_entry_pool[LEVELRENDER_POOL_SIZE]; // 0x22ec4e4
int levelrender_22f8ce4;                  // 0x22f8ce4 never accessed (name ours)
float levelrender_fx_char_tint_b;         // 0x22f8ce8
float hover_tint_char_b;                  // 0x22f8cec
int levelrender_22f8cfc;                  // 0x22f8cfc never accessed (name ours)
lobj *hover_tint_char;                    // 0x22f8d0c
float hover_tint_char_g;                  // 0x22f8d10
levelrender_entry *levelrender_hover_target;          // 0x22f8d14
attached_light *levelrender_level_lights[LEVELRENDER_MAX_LEVEL_LIGHTS];   // 0x22f8d18
levelrender_entry *levelrender_sort_list[LEVELRENDER_MAX_SORTED];          // 0x22f8d84
int levelrender_22f9554;                  // 0x22f9554 never accessed (name ours)
bool overhead_label_slot_used[2][3];      // 0x22f9558
character_speech_record *overhead_label_slot_owner[2][3];   // 0x22f9560
bool lamp_glows_pending;                  // 0x22f9578 (name ours)
s3d_vis_set *level_vis_set_records[LEVEL_MAX_VIS_SETS];     // 0x22f957c
lobj_render_state *levelrender_lobj_states;                 // 0x22f9d7c
levelrender_entry *levelrender_float_texts[LEVELRENDER_MAX_FLOAT_TEXTS];   // 0x22f9d8c
int levelrender_level_light_handles[LEVELRENDER_MAX_LEVEL_LIGHTS];        // 0x22f9e28
ubyte *levelrender_entry_pool_cursor;     // 0x22f9e88
s3d_vis_set *level_vis_set_lists[LEVEL_MAX_VIS_SETS];       // 0x22f9e9c (name ours)
float hover_tint_char_r;                  // 0x22fa69c
lobj_glow_ref *levelrender_lobj_glow_map; // 0x22fa6a0
obj *hover_tint_obj;                      // 0x22fa6a4
levelrender_entry *levelrender_opaque_list[LEVELRENDER_MAX_SORTED];        // 0x22fa6a8
float level_layer_fog_far[LEVEL_NUM_LAYERS];         // 0x22fae78
float hover_tint_obj_g;                   // 0x22faec8
int levelrender_lobj_state_bytes;         // 0x22faecc
int level_vis_force_count;                // 0x22faedc
float hover_tint_obj_b;                   // 0x22faee0
level_layer_vis_rec level_layer_vis[LEVEL_NUM_LAYERS];      // 0x22faee8
bool level_layer_snow[LEVEL_NUM_LAYERS];  // 0x22faffc
detail_level detail_levels[MAX_DETAIL_LEVELS];       // 0x22fb010
float levelrender_fx_char_tint_r;         // 0x22fb678
ubyte level_layer_fog_b[LEVEL_NUM_LAYERS];           // 0x22fb67c
ubyte level_layer_fog_g[LEVEL_NUM_LAYERS];           // 0x22fb690
ubyte level_layer_fog_r[LEVEL_NUM_LAYERS];           // 0x22fb6a4
int num_detail_levels;                    // 0x22fb6c0
int levelrender_attach_count;             // 0x22fb6c4
int levelrender_opaque_count;             // 0x22fb6c8
int levelrender_sort_count;               // 0x22fb6cc
Character *levelrender_fx_char;           // 0x22fb6d0
bool levelrender_draw_chain_marker;       // 0x22fb6d4
int fog_color_r;                          // 0x22fb6d8
int fog_color_g;                          // 0x22fb6dc
int fog_color_b;                          // 0x22fb6e0
float char_fog_near;                      // 0x22fb6e4
float char_fog_far;                       // 0x22fb6e8
float char_fog_near_sq;                   // 0x22fb6ec
float char_fog_far_sq;                    // 0x22fb6f0
bool char_fog_enabled;                    // 0x22fb6f4
bool fog_level_default_set;               // 0x22fb6f5
bool levelrender_22fb6f6;                 // 0x22fb6f6
bool levelrender_flag_22fb6f7;            // 0x22fb6f7
bool fog_fading;                          // 0x22fb6f8
float fog_fade_amount;                    // 0x22fb6fc
int level_num_vis_sets;                   // 0x22fb700
int levelrender_num_level_lights;         // 0x22fb704
bool level_vis_dirty;                     // 0x22fb708
int level_vis_cache_key[2];               // 0x22fb70c
bool levelrender_no_lighting;             // 0x22fb714
bool level_vis_forced;                    // 0x22fb715
bool Cull_ground;                         // 0x22fb716
bool levelrender_fade_in;                 // 0x22fb717
bool levelrender_layer_fog_applied;       // 0x22fb718

// The bump allocator of the draw entries (inline; the cursor moves on even when the pool is full).
static inline levelrender_entry *levelrender_alloc_entry(int size)
{
    ubyte *e = levelrender_entry_pool_cursor;
    levelrender_entry_pool_cursor += size;
    if (levelrender_entry_pool_cursor - levelrender_entry_pool > LEVELRENDER_POOL_SIZE)
        return NULL;
    return (levelrender_entry *)e;
}

// ---- console commands --------------------------------------------------------------------------------
// 0x4816e0 init_ccmd_lod_dist_thunk: jmp init_ccmd_lod_dist (static initialiser entry 242). Confidence: exact.
// 0x4816f0 init_ccmd_lod_dist: the dynamic initialiser of ccmd_lod_dist (console_command::console_command). Confidence: exact.
console_command ccmd_lod_dist("lod_dist", "Sets Character_lod_dist", cmd_lod_dist);   // 0x22ec4d8

// 0x481710  Confidence: exact (the console's float command macro).
void cmd_lod_dist()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Character_lod_dist = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "lod_dist",
                "Character_lod_dist");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "lod_dist", Character_lod_dist);
        print_to_console(string_buffer, 0);
    }
}

// 0x4817a0 init_ccmd_main_lod_dist_thunk: jmp init_ccmd_main_lod_dist (static initialiser entry 243). Confidence: exact.
// 0x4817b0 init_ccmd_main_lod_dist: the dynamic initialiser of ccmd_main_lod_dist (console_command::console_command). Confidence: exact.
console_command ccmd_main_lod_dist("main_lod_dist", "Sets Main_character_lod_dist", cmd_main_lod_dist);   // 0x22f8d78

// 0x4817d0  Confidence: exact.
void cmd_main_lod_dist()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Main_character_lod_dist = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "main_lod_dist",
                "Main_character_lod_dist");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "main_lod_dist", Main_character_lod_dist);
        print_to_console(string_buffer, 0);
    }
}

// The console's bool command macro (as cmd_geom_avoid in the camera unit).
#define LR_BOOL_COMMAND(var, name)                                                                      \
    if (calling_a_command_function) {                                                                   \
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);                                               \
        if (arg_type & ARG_TRUE)                                                                        \
            var = true;                                                                                 \
        else if (arg_type & ARG_FALSE)                                                                  \
            var = false;                                                                                \
        else if (arg_type & ARG_NONE)                                                                   \
            var ^= 1;                                                                                   \
    }                                                                                                   \
    if (doing_help_for_comand) {                                                                        \
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n", \
                name, #var);                                                                            \
        print_to_console(string_buffer, 0);                                                             \
    }                                                                                                   \
    if (checking_status_for_command) {                                                                  \
        sprintf(string_buffer, "%s is %s\n", name, var ? "TRUE" : "FALSE");                             \
        print_to_console(string_buffer, 0);                                                             \
    }

// 0x481860 init_ccmd_render_level_thunk: jmp init_ccmd_render_level (static initialiser entry 244). Confidence: exact.
// 0x481870 init_ccmd_render_level: the dynamic initialiser of ccmd_render_level (console_command::console_command). Confidence: exact.
console_command ccmd_render_level("render_level", "Toggles Render_level", cmd_render_level);   // 0x22f8cf0

// 0x481890  Confidence: exact. Nothing else reads Render_level.
void cmd_render_level()
{
    LR_BOOL_COMMAND(Render_level, "render_level")
}

// 0x481950 init_ccmd_render_characters_thunk: jmp init_ccmd_render_characters (static initialiser entry 245). Confidence: exact.
// 0x481960 init_ccmd_render_characters: the dynamic initialiser of ccmd_render_characters (console_command::console_command). Confidence: exact.
console_command ccmd_render_characters("render_characters", "Toggles Render_characters",
                                       cmd_render_characters);   // 0x22faed0

// 0x481980  Confidence: exact. Nothing else reads Render_characters.
void cmd_render_characters()
{
    LR_BOOL_COMMAND(Render_characters, "render_characters")
}

// 0x481a40 init_ccmd_render_effects_thunk: jmp init_ccmd_render_effects (static initialiser entry 246). Confidence: exact.
// 0x481a50 init_ccmd_render_effects: the dynamic initialiser of ccmd_render_effects (console_command::console_command). Confidence: exact.
console_command ccmd_render_effects("render_effects", "Toggles Render_effects", cmd_render_effects);   // 0x22f8d00

// 0x481a70  Confidence: exact. Nothing else reads Render_effects.
void cmd_render_effects()
{
    LR_BOOL_COMMAND(Render_effects, "render_effects")
}

// 0x481b30 init_ccmd_set_fog_thunk: jmp init_ccmd_set_fog (static initialiser entry 247). Confidence: exact.
// 0x481b40 init_ccmd_set_fog: the dynamic initialiser of ccmd_set_fog (console_command::console_command). Confidence: exact.
console_command ccmd_set_fog("set_fog", "Set the fogging clip plane.", cmd_set_fog);   // 0x22f9d80

// 0x481b60  Confidence: likely (the two locals; status before help, as the code tests them). "set_fog
// far [near]": without the second value the near distance is far - 10, at least 1.0. The help text
// says "<0.0f = no fog", but fog_set_distances replaces a value <= 0 by the level default.
void cmd_set_fog()
{
    float far_dist, near_dist;

    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        far_dist = command_argument_float;
        process_command(ARG_FLOAT | ARG_NONE);
        if (arg_type & ARG_FLOAT)
            near_dist = command_argument_float;
        else if (1.0f < far_dist - 10.0f)
            near_dist = far_dist - 10.0f;
        else
            near_dist = 1.0f;
        fog_set_distances(near_dist, far_dist);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Fog is currently at far: %.3f meters and near: %.3f meters.\n",
                fog_far_distance, fog_near_distance);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: set_fog <far distance> <near distance>-- where distance is in meters.  <0.0f = no fog.\n"
                         "near distance is optional.", 0);
}

// 0x481c40  Confidence: likely (the || chain; the vertex copy). The bitmap is centred on the projected
// head tag, its bottom 25 pixels above it; x is made relative to the clip origin, y is not.
void character_draw_overhead_bitmap(Character *c, int bm)
{
    vector3 pos;
    matrix orient;
    gr_vertex v;
    int clip_x, clip_y, clip_w, clip_h;
    int w, h;
    int tag;

    if (c->type == 7 || !_stricmp(c->name, "Ulsadana")
        || (fabs(cur_character->pos.y - c->pos.y) <= 4.0f && distance_squared(&cur_character->pos, &c->pos) <= 2500.0f
            && (c->obj_flags & 2))) {
        tag = c->instance->character->find_mesh_tag("BDBN-Head");
        if (tag >= 0 || (tag = c->instance->character->find_mesh_tag("Head")) >= 0) {
            c->instance->get_bone_transform(&pos, &orient, tag);
            pos.rotate(c->orient);
            pos += c->pos;
            pos.y += 0.1f;
        } else {
            pos = c->pos;
            pos.y += c->info->icon_height;
        }
        gr_rotate_vertex(&v, &pos);
        gr_project_vertex(&v);
        gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
        v.sx = v.sx - clip_x;
        bm_get_dimensions(bm, &w, &h);
        gr_bitmap(bm, (int)v.sx - w / 2, (int)v.sy - h / 2 - 25, gr_bitmap_wrap_mode);
    }
}

// 0x481e00  Confidence: exact.
void char_fog_set(float near_dist, float far_dist)
{
    char_fog_enabled = true;
    char_fog_near = near_dist;
    char_fog_far = far_dist;
    char_fog_near_sq = near_dist * near_dist;
    char_fog_far_sq = far_dist * far_dist;
}

// 0x481e40  Confidence: exact.
void char_fog_disable()
{
    char_fog_enabled = false;
}

// 0x481e50  Confidence: exact.
float char_fog_get_far()
{
    if (char_fog_enabled)
        return char_fog_far;
    return 0.0f;
}

// 0x481e70 init_ccmd_Cull_ground_thunk: jmp init_ccmd_Cull_ground (static initialiser entry 248). Confidence: exact.
// 0x481e80 init_ccmd_Cull_ground: the dynamic initialiser of ccmd_Cull_ground (console_command::console_command). Confidence: exact.
console_command ccmd_Cull_ground("Cull_ground", "Toggles Cull_ground", cmd_Cull_ground);   // 0x22ec4c8

// 0x481ea0  Confidence: exact. Cull_ground is also the level fade's state (0 alpha = culled).
void cmd_Cull_ground()
{
    LR_BOOL_COMMAND(Cull_ground, "Cull_ground")
}

// 0x481f60 init_ccmd_Cull_ground_too_thunk: jmp init_ccmd_Cull_ground_too (static initialiser entry 249). Confidence: exact.
// 0x481f70 init_ccmd_Cull_ground_too: the dynamic initialiser of ccmd_Cull_ground_too (console_command::console_command). Confidence: exact.
console_command ccmd_Cull_ground_too("Cull_ground_too", "Toggles Cull_ground_too", cmd_Cull_ground_too);   // 0x22f9dd0

// 0x481f90  Confidence: exact.
void cmd_Cull_ground_too()
{
    LR_BOOL_COMMAND(Cull_ground_too, "Cull_ground_too")
}

// ---- the level fade and the hover tints ---------------------------------------------------------------
// 0x482050  Confidence: exact. A full alpha is stepped down to 0xfe so that the frame starts fading.
void levelrender_fade_out_start()
{
    if (levelrender_fade_alpha >= 0xfe)
        levelrender_fade_alpha = 0xfe;
    levelrender_fade_in = false;
}

// 0x482070  Confidence: exact.
void levelrender_fade_reset()
{
    levelrender_fade_alpha = 0xff;
    levelrender_fade_in = true;
    Cull_ground = false;
}

// 0x482090  Confidence: exact (int, D4300).
int hover_tint_active()
{
    return hover_tint_obj || hover_tint_char;
}

// 0x4820b0  Confidence: exact.
void hover_tint_set_object(obj *o, float r, float g, float b)
{
    hover_tint_obj = o;
    hover_tint_obj_r = r;
    hover_tint_obj_g = g;
    hover_tint_obj_b = b;
}

// 0x4820e0  Confidence: exact.
void hover_tint_set_character(lobj *l, float r, float g, float b)
{
    hover_tint_char = l;
    hover_tint_char_r = r;
    hover_tint_char_g = g;
    hover_tint_char_b = b;
}

// 0x482110  Confidence: likely (the case order of the switch). Once per layer change: the camera mode
// of the layer (unless the follow view is off or a sequence runs) and its fog; without a fog of its own
// the camera preset's defaults. The layer's far value goes to fog_near_distance (the same swap as
// fog_set_level_default; see NOTES).
void level_set_current_layer(int layer)
{
    level_current_layer = layer;
    if (layer != level_rendered_layer)
        levelrender_layer_fog_applied = false;
    else if (levelrender_layer_fog_applied)
        return;
    if (!(game_camera.flags & CAMF_FOLLOW_VIEW) || (game_camera.flags & CAMF_SEQUENCE))
        return;
    switch (level_layer_vis_flags[layer]) {
    case 0:
        game_camera.set_mode(4);
        break;
    case 1:
        game_camera.set_mode(3);
        break;
    }
    if (level_layer_fog_far[layer] > 0.0f) {
        fog_near_distance = level_layer_fog_far[layer];
        fog_far_distance = level_layer_fog_near[layer];
        fog_set_color(level_layer_fog_r[layer], level_layer_fog_g[layer], level_layer_fog_b[layer]);
        levelrender_layer_fog_applied = true;
        return;
    }
    if (game_camera.elevation < 4.0f) {
        fog_set_low_camera_default();
        levelrender_layer_fog_applied = true;
        return;
    }
    fog_set_high_camera_default();
    levelrender_layer_fog_applied = true;
}

// ---- the frame ----------------------------------------------------------------------------------------
// 0x482200  Confidence: likely (the scope objects of states 1-4 are placed where the EH states change;
// gr_light_desc `light` is constructed and never used). The vis sets of the current layer (or the
// cutscene's forced list), the level fade (256 alpha steps a second; at 0 the ground is culled), the
// lights, the level objects of the sets, the living entities / items / containers (single player, or a
// network game that is playing), the effects, the shadows, the sky effects (around the eye, before a
// z-buffer clear), the three lists, particles, snow, the overhead texts, then the letterbox bars.
void levelrender_render_scene()
{
    render_scope scope;
    gr_light_desc light;
    s3d_vis_set **sets;
    int count;
    int i, j;
    float h_fov, v_fov;
    vfx_node_ref ref;

    lamp_glows_pending = false;
    if (!(game_flags & 0x120))
        Gr_3d_zmult = 1.0f;
    if (level_vis_forced) {
        count = level_vis_force_count;
        sets = level_vis_force_list;
    } else if (level_current_layer >= 0 && level_current_layer < LEVEL_NUM_LAYERS) {
        if (Summoner_options.fog_quality == 0)
            sets = level_layer_vis[level_current_layer].sets;
        else
            sets = level_layer_vis[level_current_layer].sets_high;
        count = level_layer_vis[level_current_layer].count;
        if (sets == NULL)
            count = 0;
    } else {
        sets = NULL;
        count = 0;
    }

    levelrender_attach_count = 0;
    levelrender_sort_count = 0;
    levelrender_opaque_count = 0;
    levelrender_entry_pool_cursor = levelrender_entry_pool;
    levelrender_22ea498 = 0;
    if (levelrender_fade_alpha < 0xff && levelrender_fade_alpha > 0) {
        if (levelrender_fade_in == true) {
            levelrender_fade_alpha += (int)(reported_frametime * 256.0f);
            if (levelrender_fade_alpha > 0xff)
                levelrender_fade_alpha = 0xff;
        } else {
            levelrender_fade_alpha += (int)(reported_frametime * -256.0f);
            if (levelrender_fade_alpha < 0)
                levelrender_fade_alpha = 0;
        }
    }
    if (levelrender_fade_alpha == 0)
        Cull_ground = true;
    else
        Cull_ground = false;

    {
        render_scope s1;
        levelrender_num_float_texts = 0;
        levelrender_hover_target = NULL;
        gr_get_eye(&levelrender_eye_pos, &levelrender_eye_orient);
        gr_get_view_fov(&h_fov, &v_fov);
        if (levelrender_lights_dirty) {
            level_lights_setup();
            levelrender_num_level_lights = 0;
            levelrender_lights_dirty = false;
        }
        levelrender_setup_level_lights();
        lobj_face_marks_clear();
        levelrender_flag_22fb6f7 = false;
        levelrender_fog_enable();
        levelrender_fade_occluders(sets, count);
    }
    levelrender_add_lobjs(sets, count);
    if ((game_flags & 1) || mp_netgame.state >= 5)
        levelrender_build_draw_lists();
    levelrender_add_effects(sets, count);
    levelrender_sort_back_to_front(levelrender_sort_count, levelrender_sort_list);
    shadows_draw_characters(&levelrender_eye_pos, &levelrender_eye_orient, levelrender_opaque_count,
                            levelrender_opaque_list);
    shadows_draw_characters(&levelrender_eye_pos, &levelrender_eye_orient, levelrender_sort_count,
                            levelrender_sort_list);
    {
        render_scope s2;
        levelrender_fog_disable();
        for (i = 0; i < fx_num_sky; i++) {
            effect_object *fx = fx_sky_list[i];
            if (fx == NULL || Cull_ground || levelrender_fade_alpha < 0xff)
                continue;
            ref.kind = 0;
            vfx_instance *vi = fx->instance;
            if (vi == NULL)
                continue;
            vfx_base *base = effect_get_base(vi->id_bits >> 9);
            fx_rec98 *n = vi->nodes;
            int k = 0;
            if (n) {
                do {
                    if (base->node_descs[k].vis_flags & 0x600) {
                        vector3 pos = levelrender_eye_pos + fx->pos;
                        ref.index = (short)k;
                        ref.inst = n;
                        vi->render_alpha = 0xff;
                        vi->render_node(&ref, &pos, &fx->orient);
                    }
                    n = n->next;
                    k++;
                } while (n != vi->nodes && n);
            }
        }
        gr_zbuffer_clear(1);
        gr_poly_buffer_flush();
        levelrender_fog_enable();
    }
    levelrender_draw_list(levelrender_attach_count, levelrender_attach_list, 0);
    levelrender_draw_list(levelrender_opaque_count, levelrender_opaque_list, 0);
    shadows_render();
    levelrender_draw_list(levelrender_sort_count, levelrender_sort_list, 1);
    gr_set_tint(0, 0.0f, 0.0f, 0.0f);
    {
        render_scope s3;
        gr_poly_buffer_flush();
    }
    fx_draw_flagged_particles();
    if (level_current_layer >= 0 && fog_layer_snow_allowed(level_current_layer))
        snow_render();
    gr_set_color(0xff, 0xff, 0xff, 0xff);
    {
        render_scope s4;
        overhead_label_slots_expire();
        levelrender_draw_speaker_icons();
        if (!(game_flags & 0x20000)) {
            for (character_speech_record *s = character_speech; s < character_speech + MAX_CHARACTER_SPEECH; s++)
                levelrender_draw_speech_text(s);
            levelrender_draw_name_labels();
            if (levelrender_hover_target)
                levelrender_draw_health_bars(levelrender_hover_target);
            if (levelrender_draw_chain_marker) {
                gr_set_color(200, 200, 200, 0xff);
                character_draw_overhead_bitmap(cur_character, cursoricon_chain_bm);
                levelrender_draw_chain_marker = false;
            }
            for (i = 0; i < levelrender_num_float_texts; i++)
                levelrender_draw_float_text(levelrender_float_texts[i]);
        }
        if (lamp_glows_pending)
            lamp_glows_draw();
    }
    level_vis_cache_key[0] = (int)sets;
    level_vis_cache_key[1] = count;
    for (i = 0; i < count; i++) {
        s3d_vis_set *set = sets[i];
        for (j = 0; j < set->count; j++)
            lobjs[set->indices[j]].flags &= ~0x6000;
    }
    decals_recycle_unused();
    level_rendered_layer = level_current_layer;
    gr_poly_buffer_flush();
    levelrender_fog_disable();
    if (gr_byte_25d921d && (game_flags & 0x20)) {
        int top = gr_screen.offset_y;
        int bottom = gr_screen.clip_height + top;
        int rest = gr_screen.max_h - gr_screen.clip_height - top;
        gr_set_color(0, 0, 0, 0xff);
        gr::reset_clip();
        gr_rect(0, 0, gr_screen.max_w, top, gr_rect_mode, 1.0f);
        gr_rect(0, bottom, gr_screen.max_w, rest, gr_rect_mode, 1.0f);
    }
    if (d3d_s3_depth) {
        gr::reset_clip();
        gr_set_color(0, 0, 0, 0xff);
        gr_rect(0, 0, gr_screen.max_w, 3, gr_rect_mode, 1.0f);
    }
    if (!(game_flags & 0x120))
        Gr_3d_zmult = 0.2f;
}

// 0x482850  Confidence: likely (fps is loaded once for both compares). No fog when both distances are
// <= 0. fog_fading is never set, so the scaled branch is dead: it would shrink both distances by up to
// 80% while the frame rate is below 15 and grow them back above 24.
void levelrender_fog_enable()
{
    float near_dist, far_dist;
    float scale;

    if (fog_near_distance <= 0.0f && fog_far_distance <= 0.0f) {
        gr_set_fog(0, 0, 0, 0, -1.0f, -1.0f);
        return;
    }
    if (fog_fading) {
        if (fps < 15.0f)
            fog_fade_amount += 0.02f;
        if (fps > 24.0f)
            fog_fade_amount -= 0.02f;
        if (fog_fade_amount > 0.8f)
            fog_fade_amount = 0.8f;
        else if (fog_fade_amount < 0.0f)
            fog_fade_amount = 0.0f;
        scale = 1.0f - fog_fade_amount;
        near_dist = fog_near_distance * scale;
        far_dist = fog_far_distance * scale;
        if (far_dist < 15.0f)
            far_dist = 15.0f;
        gr_set_far_clip(far_dist);
        gr_set_fog(1, fog_color_r, fog_color_g, fog_color_b, near_dist, far_dist);
        return;
    }
    gr_set_far_clip(fog_far_distance);
    gr_set_fog(1, fog_color_r, fog_color_g, fog_color_b, fog_near_distance, fog_far_distance);
}

// 0x4829e0  Confidence: likely (the fade-state switch; the bit expressions follow the and / or patterns).
// Every visible level object starts the frame opaque (alpha 0xff, the occluding bit moved to the
// previous-frame bit); then the camera occluders and the members of their "$seethru" sets are marked;
// then each object fades: an occluder fades out (state 3, 270 ms down to 0x80) and stays at 0x80
// (state 1); 1 s after it stops occluding it fades back in (state 2, 270 ms) to opaque (state 0).
// A fade reversed halfway restarts its timer so that the alpha continues from where it was.
void levelrender_fade_occluders(s3d_vis_set **sets, int count)
{
    render_scope scope;
    ubyte alpha = 0xff;
    int i, j, k;

    for (i = 0; i < count; i++) {
        s3d_vis_set *set = sets[i];
        for (j = 0; j < set->count; j++) {
            int index = set->indices[j];
            lobj *l = &lobjs[index];
            if (!(l->flags & 0x4000)) {
                l->flags |= 0x4000;
                lobj_render_state *st = &levelrender_lobj_states[index];
                unsigned int f = st->flags | LRS_ALPHA_MASK;
                st->flags = ((f & 4) << 1) | (f & ~(LRS_OCCLUDING | LRS_WAS_OCCLUDING));
            }
        }
    }
    if (game_flags & 0x20)
        return;
    for (k = 0; k < camera_num_occluders; k++) {
        lobj_render_state *st = &levelrender_lobj_states[camera_occluders[k]];
        st->flags |= LRS_OCCLUDING;
        s3d_vis_set *see = levelrender_lobj_states[camera_occluders[k]].seethru;
        if (see) {
            for (j = 0; j < see->count; j++)
                levelrender_lobj_states[see->indices[j]].flags |= LRS_OCCLUDING;
        }
    }
    for (i = 0; i < count; i++) {
        s3d_vis_set *set = sets[i];
        for (j = 0; j < set->count; j++) {
            int index = set->indices[j];
            lobj_render_state *st = &levelrender_lobj_states[index];
            if (!(st->flags & 0xf))
                continue;
            lobj *l = &lobjs[index];
            if (!(l->flags & 0x4000))
                continue;
            l->flags &= ~0x4000;
            if (st->flags & LRS_OCCLUDING) {
                if ((st->flags & 3) == 0) {
                    st->timer.SetTimeout(0);
                    st->flags |= 3;
                } else if ((st->flags & 3) == 2) {
                    st->timer.SetTimeout(st->timer.GetElapsedTime() - 270 < 0 ? 0 : st->timer.GetElapsedTime() - 270);
                    st->flags |= 3;
                }
            } else if ((st->flags & 3) == 1) {
                if (st->timer.GetElapsedTime() > 1000) {
                    st->timer.SetTimeout(0);
                    st->flags = (st->flags & ~1) | 2;
                }
            } else if ((st->flags & 3) == 3) {
                if (st->timer.GetElapsedTime() > 1000) {
                    st->timer.SetTimeout(st->timer.GetElapsedTime() - 270 < 0 ? 0 : st->timer.GetElapsedTime() - 270);
                    st->flags = (st->flags & ~1) | 2;
                }
            }
            switch (st->flags & 3) {
            case 0:
                alpha = 0xff;
                break;
            case 1:
                alpha = 0x80;
                break;
            case 2: {
                int t = st->timer.GetElapsedTime();
                if (t < 270) {
                    alpha = (ubyte)(t * 127 / 270 + 0x80);
                } else {
                    st->flags &= ~3;
                    alpha = 0xff;
                }
                break;
            }
            case 3: {
                int t = st->timer.GetElapsedTime();
                if (t < 270) {
                    alpha = (ubyte)(0xff - t * 127 / 270);
                } else {
                    st->flags = (st->flags & ~2) | 1;
                    alpha = 0x80;
                }
                break;
            }
            }
            st->flags = (st->flags & ~LRS_ALPHA_MASK) | ((alpha & 0xff) << LRS_ALPHA_SHIFT);
        }
    }
}

// 0x482d40  Confidence: exact.
void levelrender_fog_disable()
{
    gr_set_fog(0, 0, 0, 0, -1.0f, -1.0f);
    gr_set_far_clip(0.0f);
}

// 0x482d70  Confidence: likely (the delete loop's form). The point lights among the first 24 attached
// light slots that have ranges become gr lights for the frame (flag 1); deleted again the next frame.
void levelrender_setup_level_lights()
{
    int count = levelrender_num_level_lights;
    int i;

    if (count) {
        for (i = count; i; i--)
            light_delete(levelrender_level_light_handles[i - 1]);
        count = 0;
    }
    if (levelrender_no_lighting) {
        levelrender_num_level_lights = 0;
        return;
    }
    for (i = 0; i < LEVELRENDER_MAX_LEVEL_LIGHTS; i++) {
        attached_light *light = level_light_get(i);
        if (light && light->has_range && light->type == 0) {
            int handle = light_create_point(&light->pos, light->range_near, light->range_far, light->intensity,
                                            light->r, light->g, light->b, 1);
            if (handle != -1) {
                levelrender_level_light_handles[count] = handle;
                levelrender_level_lights[count] = light;
                count++;
            }
        }
    }
    levelrender_num_level_lights = count;
}

// 0x482e20  Confidence: likely (the case grouping; the distance tests). Living entities (types 4-10):
// the visibility bit 2 moves to bit 8 (previous frame) and bits 2 / 0x20 are cleared; a culled monster
// or NPC within 34.6 m gets AI flag 0x40 and a culled corpse beyond 20 m is deactivated. Outside a
// cutscene an entity is drawn only within the fog distance, on a level object that is drawn this frame
// (flag 0x2000) and, when small (radius < 2), within 75 m; a living one beyond 40 m (or the fog distance
// for type 9 / species 0x55) gets flag 0x20. In a cutscene the limit is 225 m and only the cutscene's
// actors are drawn when the cutscene says so (flag 2). Attachments within 30 m (50 m with game flag 8).
// Note: character_is_in_view returns true when the sphere is culled (gr_cull_sphere's result).
void levelrender_build_draw_lists()
{
    render_scope scope;
    obj *o;

    for (o = obj_active_list.next; o != &obj_active_list; o = o->next) {
        switch (o->type) {
        case 3:
            levelrender_add_items((item_obj *)o);
            break;
        case 11:
            levelrender_add_containers((container *)o);
            break;
        case 4: case 5: case 6: case 7: case 8: case 9: case 10: {
            Character *c = (Character *)o;
            unsigned int f = c->obj_flags;
            vector3 d;
            float dist_sq;
            vector3 centre;
            float radius;
            levelrender_entry *e;

            if ((f & 4) && (c->flags & 0x8000))   // test ch, 0x80 (not CF_DEAD)
                break;
            if (f & 2)
                f |= 8;
            else
                f &= ~8;
            f &= ~0x22;
            c->obj_flags = f;
            if (!(f & 1))
                break;
            if (!(game_flags & 0x20) || c->cutscene_object < 0) {
                if (c->is_in_view(1.0f)) {
                    if (c->type == 5 || c->type == 4) {
                        if ((c->pos - cur_character->pos).len_sq() <= 1200.0f)
                            c->ai.flags |= 0x40;
                    }
                    if (c->type != 7 && (c->flags & CF_DEAD) && !((c->pos - cur_character->pos).len_sq() <= 400.0f))
                        c->obj_flags &= ~1;
                    break;
                }
            }
            if ((game_flags & 0x20) && (cutscene_data->flags & 2) && c->cutscene_object < 0)
                break;
            if (!(game_flags & 0x20) && c != cur_character) {
                if (c->ground_lobj >= num_lobjs)
                    break;
                if (c->ground_face >= 0 && !(lobjs[c->ground_lobj].flags & 0x2080))
                    break;
            }
            d.x = c->pos.x - levelrender_eye_pos.x;
            d.y = c->pos.y - levelrender_eye_pos.y;
            d.z = c->pos.z - levelrender_eye_pos.z;
            dist_sq = d.z * d.z + d.y * d.y + d.x * d.x;
            if (!(game_flags & 0x20)) {
                if (!(fog_far_distance * fog_far_distance > dist_sq))
                    break;
                c->instance->get_bounding_sphere(&centre, &radius);
                if (radius < 2.0f && !(dist_sq < 5625.0f))
                    break;
                float limit = 1600.0f;
                if (c->type == 9 || c->info->species == 0x55)
                    limit = fog_far_distance * fog_far_distance;
                if (levelrender_mark_far_living && dist_sq > limit && c->hp > 0.0f)
                    c->obj_flags |= 0x20;
            } else if (!(dist_sq < 50625.0f)) {
                break;
            }
            e = levelrender_alloc_entry(sizeof(levelrender_entry));
            if (e == NULL)
                break;
            if (c->instance->_unkb4 != 0xff)
                levelrender_sort_list[levelrender_sort_count++] = e;
            else
                levelrender_opaque_list[levelrender_opaque_count++] = e;
            e->type = LRE_CHARACTER;
            e->object = c;
            e->depth = levelrender_eye_orient.fvec.z * d.z + levelrender_eye_orient.fvec.y * d.y
                     + d.x * levelrender_eye_orient.fvec.x;
            e->dist = e->depth - c->bound_radius;
            if (c->type != 1 && c->type != 3) {
                if (((game_flags & 8) && dist_sq < 2500.0f) || (c->is_dead_type() && dist_sq < 400.0f)
                    || dist_sq < 900.0f)
                    levelrender_add_character(c);
            }
            c->obj_flags |= 2;
            break;
        }
        }
    }
}

// 0x483260  Confidence: likely. In level 24 a character in the servant disguise draws no props. The
// weapon (slot 5, unless the prop tag is 0x11), slot 9 at tag 1, slot 6 at tag 15 or 1 (not for the party
// in a cutscene), then the character's attachment list. The Ghost Rider (species 0x5a) test in the
// two ghost cutscenes can never be true: it asks for both script names at once (game bug, NOTES).
void levelrender_add_character(Character *c)
{
    int i;

    if (cur_level_index == 0x18 && character_wearing_servant_disguise(c))
        return;
    if (c->type == 7 || c->type == 5 || c->type == 4 || c->is_dead_type()) {
        if (c->equipment[5] && c->prop_tag != 0x11)
            levelrender_add_bone_attachment(c, c->equipment[5]->type, c->prop_tag);
        if (c->equipment[9])
            levelrender_add_bone_attachment(c, c->equipment[9]->type, 1);
        if (!(game_flags & 0x20) || c->type != 7) {
            if (c->equipment[6]) {
                if (c->info->prop_tag_bones[15] >= 0)
                    levelrender_add_bone_attachment(c, c->equipment[6]->type, 15);
                else if (c->info->prop_tag_bones[1] >= 0)
                    levelrender_add_bone_attachment(c, c->equipment[6]->type, 1);
            }
        }
    }
    if (c->info->species == 0x5a && (game_flags & 0x20)
        && !_stricmp(cutscene_script_table(), "Ghost-Escape_cscript.tbl")
        && !_stricmp(cutscene_script_table(), "Ghost-Dismount_cscript.tbl"))
        return;
    for (i = 0; i < c->num_attachments; i++) {
        if (c->attachments[i].slot != 0x11)
            levelrender_add_bone_attachment(c, &item_types[c->attachments[i].item_type], c->attachments[i].slot);
    }
}

// 0x4833b0  Confidence: likely (the alpha is a byte stored over the dead first argument's slot and pushed
// as a dword: the parameter is a char type).
void levelrender_add_bone_attachment(Character *c, ItemType *item, int tag)
{
    vector3 pos;
    matrix orient;
    int bone = c->info->prop_tag_bones[tag];

    if (bone == -1)
        return;
    c->instance->get_bone_world_transform(bone, &c->orient, &c->pos, &orient, &pos);
    ubyte alpha = c->get_move_fade_alpha();
    levelrender_push_attachment(item, c, &pos, &orient, alpha, c->light_level);
}

// 0x483430  Confidence: likely (the depth uses the mesh's bounding centre without the bone transform;
// the alpha argument is not read: levelrender_draw_list takes it from the character again).
void levelrender_push_attachment(ItemType *item, Character *c, const vector3 *pos, const matrix *orient,
                                 ubyte alpha, float light)
{
    levelrender_attach_entry *e;

    if (!item || !c || !item->mesh)
        return;
    e = (levelrender_attach_entry *)levelrender_alloc_entry(sizeof(levelrender_attach_entry));
    if (e == NULL)
        return;
    levelrender_attach_list[levelrender_attach_count++] = e;
    e->light = light;
    e->type = LRE_ATTACHMENT;
    e->item = item;
    e->character = c;
    e->pos = *pos;
    e->orient = *orient;
    vim *mesh = item->mesh;
    float cy = mesh->center.y, cz = mesh->center.z;
    e->depth = (cz - levelrender_eye_pos.z) * levelrender_eye_orient.fvec.z
             + (cy - levelrender_eye_pos.y) * levelrender_eye_orient.fvec.y
             + (mesh->center.x - levelrender_eye_pos.x) * levelrender_eye_orient.fvec.x;
    e->dist = e->depth - mesh->radius;
}

// 0x483570  Confidence: likely. Only "Gold" item objects are drawn from here, and only those lying in
// the level within 20 m, in view, and (outside a cutscene) on a level object drawn this frame.
void levelrender_add_items(item_obj *it)
{
    vector3 d;
    levelrender_entry *e;

    d.x = it->pos.x - levelrender_eye_pos.x;
    d.y = it->pos.y - levelrender_eye_pos.y;
    d.z = it->pos.z - levelrender_eye_pos.z;
    if (it->in_container || !(it->obj_flags & 1))
        return;
    if (_stricmp(it->item->type->name, "Gold"))
        return;
    if (d.z * d.z + d.y * d.y + d.x * d.x > 400.0f)
        return;
    if (it->is_in_view())
        return;
    if (!(game_flags & 0x20) && it->ground_face >= 0) {
        if (it->ground_face >= num_lobjs || !(lobjs[it->ground_face].flags & 0x2080))
            return;
    }
    if (it->obj_flags & 2)
        it->obj_flags |= 8;
    else
        it->obj_flags &= ~8;
    e = levelrender_alloc_entry(sizeof(levelrender_entry));
    if (e == NULL)
        return;
    levelrender_attach_list[levelrender_attach_count++] = e;
    e->type = LRE_ITEM;
    e->object = it;
    e->depth = levelrender_eye_orient.fvec.z * d.z + levelrender_eye_orient.fvec.x * d.x
             + levelrender_eye_orient.fvec.y * d.y;
    e->dist = e->depth - it->bound_radius;
    it->obj_flags |= 2;
}

// 0x4836f0  Confidence: likely. A container within 20 m and in view; its floor object is the first
// item's (-1 when empty).
void levelrender_add_containers(container *ct)
{
    vector3 d;
    int face;
    levelrender_entry *e;

    if (!(ct->obj_flags & 1))
        return;
    d.x = ct->pos.x - levelrender_eye_pos.x;
    d.y = ct->pos.y - levelrender_eye_pos.y;
    d.z = ct->pos.z - levelrender_eye_pos.z;
    if (d.z * d.z + d.y * d.y + d.x * d.x > 400.0f)
        return;
    if (ct->is_in_view())
        return;
    face = ct->num_items > 0 ? ct->items[0]->ground_face : -1;
    if (!(game_flags & 0x20) && face >= 0) {
        if (face >= num_lobjs || !(lobjs[face].flags & 0x2080))
            return;
    }
    if (ct->obj_flags & 2)
        ct->obj_flags |= 8;
    else
        ct->obj_flags &= ~8;
    e = levelrender_alloc_entry(sizeof(levelrender_entry));
    if (e == NULL)
        return;
    levelrender_attach_list[levelrender_attach_count++] = e;
    e->type = LRE_CONTAINER;
    e->object = ct;
    e->depth = levelrender_eye_orient.fvec.z * d.z + levelrender_eye_orient.fvec.x * d.x
             + levelrender_eye_orient.fvec.y * d.y;
    e->dist = e->depth - ct->bound_radius;
    ct->obj_flags |= 2;
}

// 0x483850  Confidence: likely (the scope object of the EH frame). Each object of the sets once
// (flag 0x2000, cleared by levelrender_render_scene at the end of the frame).
void levelrender_add_lobjs(s3d_vis_set **sets, int count)
{
    render_scope scope;
    int i, j;

    for (i = 0; i < count; i++) {
        s3d_vis_set *set = sets[i];
        for (j = 0; j < set->count; j++) {
            int index = set->indices[j];
            lobj *l = &lobjs[index];
            if (!(l->flags & 0x2000)) {
                l->flags |= 0x2000;
                levelrender_add_lobj(l, index);
            }
        }
    }
}

// 0x4838f0  Confidence: likely. Fully transparent objects are skipped. A moving door grows its sphere
// by the distance between its two poses; doors are tested against the sphere only, the rest against
// the sphere and the box (small-mesh culling below Small_mesh_threshold). The distance limits are
// applied only with levelrender_no_lighting (never set). Partly faded objects go to the sorted list,
// objects with alpha faces to the opaque list, the rest to the attach list. A lamp object fades its
// glow out here; lamp_glow_update_lobjs fades it in again when the eye sees it.
void levelrender_add_lobj(lobj *l, int index)
{
    lobj_render_state *states = levelrender_lobj_states;
    vector3 d;
    float radius;
    levelrender_entry *e;

    if (!(states[index].flags & LRS_ALPHA_MASK))
        return;
    radius = l->mesh->_unk30;
    if (l->flags & 0x100) {
        door_info *door = door_from_lobj(l);
        if (door->moving)
            radius += (door->closed_pos - door->open_pos).len();
        if (gr_cull_sphere(&l->centre, radius, l->mesh->_unk30 > Small_mesh_threshold))
            return;
    } else {
        if (gr_cull_sphere(&l->centre, radius, radius > Small_mesh_threshold))
            return;
        if (gr_cull_bounding_box(&l->bbox_min, &l->bbox_max))
            return;
    }
    d.x = l->centre.x - levelrender_eye_pos.x;
    d.y = l->centre.y - levelrender_eye_pos.y;
    d.z = l->centre.z - levelrender_eye_pos.z;
    if (levelrender_no_lighting) {
        float dist_sq = d.z * d.z + d.y * d.y + d.x * d.x - radius * radius;
        if (!(dist_sq < fog_far_distance * fog_far_distance))
            return;
        if (radius < 3.0f) {
            if (!(dist_sq < 400.0f))
                return;
        } else if (radius < 8.0f) {
            if (!(dist_sq < 900.0f))
                return;
        }
    }
    e = levelrender_alloc_entry(sizeof(levelrender_entry));
    if (e == NULL)
        return;
    if ((levelrender_lobj_states[index].flags & LRS_ALPHA_MASK) != LRS_ALPHA_MASK)
        levelrender_sort_list[levelrender_sort_count++] = e;
    else if (l->flags & 0x10)
        levelrender_opaque_list[levelrender_opaque_count++] = e;
    else
        levelrender_attach_list[levelrender_attach_count++] = e;
    e->type = LRE_LOBJ;
    e->lobj = (short)index;
    e->depth = d.z * levelrender_eye_orient.fvec.z + levelrender_eye_orient.fvec.y * d.y
             + levelrender_eye_orient.fvec.x * d.x;
    e->dist = e->depth - radius;
    if (l->flags & 8) {
        lamp_glow_fade_out(levelrender_lobj_glow_map[index].glow, reported_frametime);
        lamp_glows_pending = true;
    }
}

// 0x483ba0  Confidence: likely (the three record loops). For every active effect: a line trail
// (bits 1 | 8) is one entry; otherwise the $vis tags are reapplied when the vis sets changed, hidden
// effects (bit 4) and the inactive owner's own character effect are skipped, the instance sphere is
// culled (bit 2), then the shown nodes (glow nodes refresh their lamp glow), the vif particle records
// (updated unless paused, marked for fx_draw_flagged_particles) and the emitters (vfx_emitter_desc) become entries.
void levelrender_add_effects(s3d_vis_set **sets, int count)
{
    render_scope scope;
    effect_object *fx;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (!(fx->obj_flags & 1))
            continue;
        if (fx->bits & 1) {
            if (fx->bits & 8)
                levelrender_add_effect_entry(fx->pos, 6.0f, 6.0f, fx, 6, 0, NULL, true);
            continue;
        }
        if (level_vis_dirty || level_vis_cache_key[0] != (int)sets || level_vis_cache_key[1] != count)
            levelrender_effect_apply_vis(fx, sets, count);
        if (fx->bits & 4)
            continue;
        vfx_base *base = effect_get_base(fx->instance->id_bits >> 9);
        if (base == NULL)
            continue;
        if (fx->kind == 1 || fx->kind == 2) {
            Character *owner = living_from_handle(fx->owner);
            if (owner && !(owner->obj_flags & 1) && owner->info->vfx && !_stricmp(base->name, owner->info->vfx))
                continue;
        }
        vfx_instance *vi = fx->instance;
        if (fx->bits & 2) {
            vector3 c = vi->pos.unrotated(fx->orient);
            c.x = fx->pos.x + c.x;
            c.y = fx->pos.y + c.y;
            c.z = fx->pos.z + c.z;
            if (gr_cull_sphere(&c, vi->_unk3c, 0))
                continue;
        }
        fx_rec98 *n = vi->nodes;
        int index = 0;
        if (n) {
            do {
                vfx_node_desc *rec = &base->node_descs[index];
                if (rec->vis_flags & 0x600) {
                    if (levelrender_add_effect_entry(rec->pos, rec->cull_radius, rec->glow_radius, fx, 0, index, n,
                                                     !(fx->instance->id_bits & 0x80))
                        && (rec->flags & 0x40)) {
                        lamp_glow_fade_out(n->glow, reported_frametime);
                        lamp_glow_set(n->glow, &rec->pos, rec->glow_radius);
                    }
                }
                index++;
                n = n->next;
            } while (n != vi->nodes && n);
        }
        vfx_inst21c *em = vi->insts21c;
        if (em) {
            int k = 0;
            do {
                vfx_desc21c *erec = &base->descs21c[k];
                if (erec->vis_flags & 0x180) {
                    vector3 centre;
                    float radius;
                    vif_record_get_bounds(erec, &centre, &radius);
                    if (!(em->flags & 0x10) && !(fx->instance->id_bits & 0x80)) {
                        vector3 c = centre.unrotated(fx->orient);
                        c.x = fx->pos.x + c.x;
                        c.y = fx->pos.y + c.y;
                        c.z = fx->pos.z + c.z;
                        if (gr_cull_sphere(&c, radius, 0))
                            goto next_emitter;
                    }
                    if (!game_is_paused())
                        vif_emitter_update(em, &base->descs21c[k], 1, &fx->pos, &fx->orient, vi->_unk0c);
                    em->flags |= 0x10;
                }
            next_emitter:
                em = em->next;
                k++;
            } while (em != vi->insts21c && em);
        }
        fx_recb0 *m = vi->emitters;
        if (m) {
            int k = 0;
            do {
                vfx_emitter_desc *mrec = &base->emitters[k];
                levelrender_add_effect_entry(mrec->centre, mrec->radius, mrec->radius, fx, 4, k, m, false);
                m = m->next;
                k++;
            } while (m != vi->emitters && m);
        }
    }
    level_vis_dirty = false;
}

// 0x483f90  Confidence: likely (the "HIGH" name built in a 0x40-byte buffer). The $vis tags of the
// effect: everything not always shown is hidden, then the records of each tag named like one of the
// visible sets (or its "$visHIGH..." variant written without HIGH) are shown again.
void levelrender_effect_apply_vis(effect_object *fx, s3d_vis_set **sets, int count)
{
    char name[0x40];
    vfx_base *base = effect_get_base(fx->instance->id_bits >> 9);
    int i, j, k;

    for (i = 0; i < base->num_emitters; i++) {
        if (!(base->emitters[i].vis_flags & 0x800000))
            base->emitters[i].vis_flags &= ~0x400000;
    }
    for (i = 0; i < base->num_nodes; i++) {
        if (!(base->node_descs[i].vis_flags & 0x400))
            base->node_descs[i].vis_flags &= ~0x200;
    }
    for (i = 0; i < base->num_desc21c; i++) {
        if (!(base->descs21c[i].vis_flags & 0x80))
            base->descs21c[i].vis_flags &= ~0x100;
    }
    for (i = 0; i < base->num_tags; i++) {
        vfx_base_tag *tag = &base->tags[i];
        if (_strnicmp(tag->name, "$vis", 4) || count <= 0)
            continue;
        for (j = 0; j < count; j++) {
            s3d_vis_set *set = sets[j];
            if (!_strnicmp(set->name + 4, "high", 4)) {
                strcpy(name, "$vis");
                strcat(name, set->name + 8);
            } else {
                strcpy(name, set->name);
            }
            if (_stricmp(tag->name, set->name) && _stricmp(tag->name, name))
                continue;
            for (k = 0; k < tag->count; k++) {
                switch (tag->refs[k].kind) {
                case 0:
                    ((vfx_node_desc *)tag->refs[k].rec)->vis_flags |= 0x200;
                    break;
                case 1:
                    ((vfx_emitter_desc *)tag->refs[k].rec)->vis_flags |= 0x400000;
                    break;
                case 2:
                    ((vfx_desc21c *)tag->refs[k].rec)->vis_flags |= 0x100;
                    break;
                }
            }
        }
    }
}

// 0x4841e0  Confidence: likely (the list push comes before the NULL test of the new entry, as in the
// code). The position is in the effect's frame. With `cull` (the nodes of instances without bit 0x80)
// an effect owned by a living entity is drawn exactly when the owner is (a summon's effects only after
// the effect's timer has run out), any other effect when its sphere is in view; trails (bit 8) and
// kind-4 effects are never culled. Opaque effects (bit 2) put their nodes into the opaque list.
bool levelrender_add_effect_entry(vector3 pos, float cull_radius, float sort_radius, effect_object *fx,
                                  int kind, int index, void *inst, bool cull)
{
    levelrender_effect_entry *e;

    pos.unrotate(fx->orient);
    pos.x += fx->pos.x;
    pos.y += fx->pos.y;
    pos.z += fx->pos.z;
    if (levelrender_no_lighting)
        return false;
    if (cull && !(fx->bits & 8) && fx->kind != 4) {
        if (fx->owner > 0) {
            Character *owner = living_from_handle(fx->owner);
            if (owner && (!(owner->flags & CF_SUMMON) || fx->timer.elapsed())) {
                if (!(owner->obj_flags & 2))
                    return false;
                goto add;
            }
        }
        if (gr_cull_sphere(&pos, cull_radius, 0))
            return false;
    }
add:
    e = (levelrender_effect_entry *)levelrender_alloc_entry(sizeof(levelrender_effect_entry));
    if (e) {
        if ((fx->bits & 2) && !(ubyte)kind)
            levelrender_opaque_list[levelrender_opaque_count++] = e;
        else
            levelrender_sort_list[levelrender_sort_count++] = e;
    }
    if (!e)
        return false;
    float dx = pos.x - levelrender_eye_pos.x;
    float dy = pos.y - levelrender_eye_pos.y;
    float dz = pos.z - levelrender_eye_pos.z;
    e->type = LRE_EFFECT;
    e->fx = fx;
    e->ref.kind = (ubyte)kind;
    e->ref.index = (short)index;
    e->ref.inst = inst;
    e->depth = dx * levelrender_eye_orient.fvec.x + levelrender_eye_orient.fvec.z * dz
             + levelrender_eye_orient.fvec.y * dy;
    e->dist = e->depth - sort_radius;
    return true;
}

// ---- lamp glows ---------------------------------------------------------------------------------------
// 0x484380  Confidence: likely (the scope object of the EH frame).
void lamp_glows_draw()
{
    render_scope scope;

    if (levelrender_no_lighting)
        return;
    gr_set_color(0xff, 0xff, 0xff, 0xff);
    lamp_glow_update_lobjs();
    lamp_glow_update_effects();
}

// 0x4843e0  Confidence: likely. For each lamp object in the attach list within 40 m of the eye: a ray
// from its centre to the eye, against the object that hid it last, then (that one missing) every level
// object of the attach list from the end; the hiding object is remembered. A free view fades the glow in.
void lamp_glow_update_lobjs()
{
    int i, j;
    vector3 centre;
    float radius;

    for (i = 0; i < levelrender_attach_count; i++) {
        levelrender_entry *e = levelrender_attach_list[i];
        if (e->type != LRE_LOBJ || !(lobj_from_index(e->lobj)->flags & 8))
            continue;
        lobj_ray ray;
        lobj_hit hit;
        lobj_get_bounding_sphere(e->lobj, &centre, &radius);
        int occluder = levelrender_lobj_glow_map[e->lobj].occluder;
        ray.dir.x = levelrender_eye_pos.x - centre.x;
        ray.dir.y = levelrender_eye_pos.y - centre.y;
        ray.dir.z = levelrender_eye_pos.z - centre.z;
        ray.start = centre;
        if (ray.dir.x * ray.dir.x + ray.dir.y * ray.dir.y + ray.dir.z * ray.dir.z > 1600.0f)
            continue;
        ray.flags = 1;
        ray.require_flags = 0;
        ray.exclude_flags = 0;
        ray._unk18 = 0.0f;
        hit.num_hits = 0;
        if (occluder == -1 || (lobj_raycast(occluder, &ray, &hit, 1), hit.num_hits == 0)) {
            for (j = levelrender_attach_count - 1; j >= 0; j--) {
                if (j == i)
                    continue;
                levelrender_entry *o = levelrender_attach_list[j];
                if (o->type != LRE_LOBJ)
                    continue;
                if (lobj_index_out_of_range(o->lobj) == true)
                    break;
                if (o->lobj == occluder)
                    continue;
                lobj_raycast(o->lobj, &ray, &hit, 1);
                if (hit.num_hits) {
                    levelrender_lobj_glow_map[e->lobj].occluder = o->lobj;
                    break;
                }
            }
        }
        lobj_glow_ref *ref = &levelrender_lobj_glow_map[e->lobj];
        int glow = ref->glow;
        if (!hit.num_hits) {
            ref->occluder = -1;
            lamp_glow_fade_in(glow, reported_frametime);
        }
        lamp_glow_draw(glow);
    }
}

// 0x484600  Confidence: likely. The same for the glow nodes of effects in the sorted list (node entries
// whose base node has flag 0x40, of effects without the trail bit), against every level object of the
// attach list. The skipped index compares the attach-list index with the sorted-list index (a slip of
// the original: one unrelated attach entry is not tested; NOTES).
void lamp_glow_update_effects()
{
    int i, j;

    for (i = 0; i < levelrender_sort_count; i++) {
        levelrender_effect_entry *e = (levelrender_effect_entry *)levelrender_sort_list[i];
        if (e->type != LRE_EFFECT || (e->fx->bits & 8) || e->ref.kind)
            continue;
        vfx_base *base = effect_get_base(e->fx->instance->id_bits >> 9);
        vfx_node_desc *rec = &base->node_descs[(ushort)e->ref.index];
        if (!(rec->flags & 0x40))
            continue;
        lobj_ray ray;
        lobj_hit hit;
        vector3 p = rec->glow_pos;
        p.unrotate(e->fx->orient);
        p.x += e->fx->pos.x;
        p.y += e->fx->pos.y;
        p.z += e->fx->pos.z;
        ray.start = p;
        ray.dir.x = levelrender_eye_pos.x - p.x;
        ray.dir.y = levelrender_eye_pos.y - p.y;
        ray.dir.z = levelrender_eye_pos.z - p.z;
        if (ray.dir.x * ray.dir.x + ray.dir.y * ray.dir.y + ray.dir.z * ray.dir.z > 1600.0f)
            continue;
        ray.flags = 1;
        ray.require_flags = 0;
        ray.exclude_flags = 0;
        ray._unk18 = 0.0f;
        hit.num_hits = 0;
        for (j = levelrender_attach_count - 1; j >= 0; j--) {
            if (j == i)
                continue;
            levelrender_entry *o = levelrender_attach_list[j];
            if (o->type != LRE_LOBJ)
                continue;
            if (lobj_index_out_of_range(o->lobj) == true)
                break;
            lobj_raycast(o->lobj, &ray, &hit, 1);
            if (hit.num_hits)
                break;
        }
        int glow = ((fx_rec98 *)e->ref.inst)->glow;
        if (!hit.num_hits)
            lamp_glow_fade_in(glow, reported_frametime);
        lamp_glow_draw(glow);
    }
}

// 0x484830  Confidence: exact. A Shell sort, largest sort key first.
void levelrender_sort_back_to_front(int count, levelrender_entry **list)
{
    int gap, i, j;

    if (count <= 1)
        return;
    for (gap = count / 2; gap > 0; gap /= 2) {
        for (i = gap; i < count; i++) {
            for (j = i - gap; j >= 0; j -= gap) {
                levelrender_entry *a = list[j];
                if (!(a->dist < list[j + gap]->dist))
                    break;
                list[j] = list[j + gap];
                list[j + gap] = a;
            }
        }
    }
}

// ---- overhead texts -----------------------------------------------------------------------------------
// 0x4848c0  Confidence: likely. The hovered monster (with a team), party member or type-9 entity within
// 50 m: a 32 x 5 bar 16 pixels left of the head, dark red, the HP fraction in red.
void levelrender_draw_health_bars(levelrender_entry *e)
{
    Character *c = (Character *)e->object;
    vector3 p;
    gr_vertex v;
    int clip_x, clip_y, clip_w, clip_h;

    if (c->type != 5 && c->type != 7 && c->type != 9)
        return;
    if (game_flags & 0x20)
        return;
    if (c->type == 5 && !c->get_team())
        return;
    if (!(c->obj_flags & 2))
        return;
    if (fabs(cur_character->pos.y - c->pos.y) > 4.0f || distance_squared(&cur_character->pos, &c->pos) > 2500.0f)
        return;
    p = e->object->pos;
    p.y = c->size_height() + p.y;
    if (c->type != 9)
        p.y += 0.75f;
    gr_rotate_vertex(&v, &p);
    gr_project_vertex(&v);
    v.sx = v.sx - 16.0f;
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    v.sx = v.sx - clip_x;
    gr_set_color(0x40, 0, 0, 0xff);
    gr_rect((int)v.sx, (int)v.sy, 32, 5, gr_rect_mode, 1.0f);
    if (c->hp > 0.0f) {
        int max_hp = c->get_max_stat(0);
        float w = c->hp / (float)max_hp * 32.0f;
        gr_set_color(0xff, 0, 0, 0xff);
        gr_rect((int)v.sx, (int)v.sy, (int)w, 5, gr_rect_mode, 1.0f);
    }
}

// 0x484aa0  Confidence: exact. Slots not used this frame lose their owner; then all are free again.
void overhead_label_slots_expire()
{
    int side, row;

    for (side = 0; side < 2; side++) {
        for (row = 0; row < 3; row++) {
            if (!overhead_label_slot_used[side][row])
                overhead_label_slot_owner[side][row] = NULL;
        }
    }
    memset(overhead_label_slot_used, 0, sizeof(overhead_label_slot_used));
}

// 0x484ae0  Confidence: likely (the || chain as in character_draw_overhead_bitmap). Outside cutscenes and
// conversations: a speech line keeps its slot, or a new slot is placed from the projected head and the
// position clamped into its sixth of the screen (two columns, three rows of (height - 120) / 3).
void levelrender_draw_speech_text(character_speech_record *s)
{
    vector3 p;
    gr_vertex v;
    int clip_x, clip_y, clip_w, clip_h;
    int w, h, name_w, name_h, max_w, total_h;
    int side, row;
    int x, y;
    char name[0x100];

    if (game_flags & 0x20)
        return;
    Character *c = living_from_handle(s->speaker);
    if (!c)
        return;
    if (!conversation_active.is_idle())
        return;
    if (c->type == 7 || !_stricmp(c->name, "Ulsadana")) {
        if (c->type == 7 && (c->flags & CF_DEAD))
            return;
    } else if (fabs(cur_character->pos.y - c->pos.y) > 4.0f || distance_squared(&cur_character->pos, &c->pos) > 2500.0f
               || !(c->obj_flags & 2)) {
        return;
    }
    p = c->pos;
    p.y = c->size_height() + p.y + 0.3f;
    gr_rotate_vertex(&v, &p);
    gr_project_vertex(&v);
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    v.sx = v.sx - clip_x;
    for (side = 0; side < 2; side++) {
        for (row = 0; row < 3; row++) {
            if (overhead_label_slot_owner[side][row] == s) {
                overhead_label_slot_used[side][row] = true;
                return;
            }
        }
    }
    gr_get_string_size(&w, &h, s->text, -1, -1);
    c->get_display_name(name);
    name_w = 0;
    name_h = 0;
    if (strlen(name)) {
        strcat(name, ":");
        gr_get_string_size(&name_w, &name_h, name, -1, -1);
    }
    total_h = name_h + h;
    max_w = max(w, name_w);
    overhead_label_place((int)v.sx, (int)v.sy, total_h, &side, &row);
    overhead_label_slot_owner[side][row] = s;
    overhead_label_slot_used[side][row] = true;
    x = (int)v.sx - max_w / 2;
    y = (int)v.sy - total_h;
    int screen_h = gr_screen_height() - 120;
    int screen_w = gr_screen_width();
    int third = screen_h / 3;
    int y_min = third * row + 10;
    int y_max = (row + 1) * third - total_h - 10;
    int half = screen_w / 2;
    int x_min = half * side + 10;
    int x_max = (side + 1) * half - max_w - 10;
    if (x_min > x)
        x = x_min;
    if (x_max < x)
        x = x_max;
    if (y_min > y)
        y = y_min;
    if (y_max < y)
        y = y_max;
    overhead_label_positions[side][row].x = (float)x;
    overhead_label_positions[side][row].y = (float)y;
}

// 0x484e30  Confidence: likely (the max / min / abs macro uses, each test evaluated twice as the code
// does). The column is the side of the screen centre, the row the third of (height - 120); a used
// slot moves to the other column when it is nearer than the row boundary, else up or down a row first.
void overhead_label_place(int x, int y, int h, int *side, int *row)
{
    int screen_h = gr_screen_height() - 120;
    int half = (gr_screen_width() - 20) / 2;
    int dx, dy;
    int s, r, s2;

    if (x < half) {
        *side = 0;
        dx = half - x;
    } else {
        *side = 1;
        dx = x - half;
    }
    int third = screen_h / 3;
    if (y < third) {
        dy = third - y;
        *row = 1;
        if (y > h)
            *row = 0;
    } else if (y < screen_h * 2 / 3) {
        dy = min(screen_h * 2 / 3 - y, y - third);
        *row = 0;
        if (y - third > h)
            *row = 1;
    } else {
        dy = y - screen_h * 2 / 3;
        *row = 1;
        if (dy > h)
            *row = 2;
    }
    s = *side;
    r = *row;
    if (!overhead_label_slot_used[s][r])
        return;
    if (dx < dy) {
        s2 = abs(s - 1);
        if (!overhead_label_slot_used[s2][r]) {
            *side = s2;
            return;
        }
        if (!overhead_label_slot_used[s][max(r - 1, 0)]) {
            *row = max(r - 1, 0);
            return;
        }
        if (!overhead_label_slot_used[s][min(r + 1, 2)]) {
            *row = min(r + 1, 2);
            return;
        }
        *side = s2;
        r = *row;
        if (!overhead_label_slot_used[s2][max(r - 1, 0)]) {
            *row = max(r - 1, 0);
            return;
        }
        if (!overhead_label_slot_used[s2][min(r + 1, 2)]) {
            *row = min(r + 1, 2);
            return;
        }
        *side = abs(s2 - 1);
        return;
    }
    if (!overhead_label_slot_used[s][max(r - 1, 0)]) {
        *row = max(r - 1, 0);
        return;
    }
    if (!overhead_label_slot_used[s][min(r + 1, 2)]) {
        *row = min(r + 1, 2);
        return;
    }
    s2 = abs(s - 1);
    *side = s2;
    r = *row;
    if (!overhead_label_slot_used[s2][r])
        return;
    if (!overhead_label_slot_used[s2][max(r - 1, 0)]) {
        *row = max(r - 1, 0);
        return;
    }
    if (!overhead_label_slot_used[s2][min(r + 1, 2)]) {
        *row = min(r + 1, 2);
        return;
    }
    *side = abs(s2 - 1);
}

// 0x4850a0  Confidence: likely. Each used slot: the speaker's incidental icon when visible, then a dark
// box with the name (red) and the line (grey) at the slot's position.
void levelrender_draw_name_labels()
{
    int side, row;
    char name[0x100];

    gr_set_color(0xff, 0xff, 0xff, 0xff);
    for (side = 0; side < 2; side++) {
        for (row = 0; row < 3; row++) {
            if (!overhead_label_slot_used[side][row])
                continue;
            character_speech_record *s = overhead_label_slot_owner[side][row];
            Character *c = living_from_handle(s->speaker);
            if (c->obj_flags & 2)
                character_draw_overhead_bitmap(c, cursoricon_incidental_bm);
            int w, h;
            gr_get_string_size(&w, &h, s->text, -1, -1);
            c->get_display_name(name);
            bool has_name = strlen(name) != 0;
            int name_w = 0, name_h = 0;
            if (has_name) {
                strcat(name, ":");
                gr_get_string_size(&name_w, &name_h, name, -1, -1);
            }
            int max_w = max(w, name_w);
            float px = overhead_label_positions[side][row].x;
            float py = overhead_label_positions[side][row].y;
            if (has_name) {
                gr_set_color(0, 0, 0, 190);
                int x = (int)px;
                int y = (int)py;
                gr_rect(x, y, max_w + 8, name_h + 3, gr_rect_mode, 1.0f);
                gr_rect(x, y + name_h + 3, max_w + 8, h + 6, gr_rect_mode, 1.0f);
                gr_set_color(200, 100, 100, 0xff);
                gr_string(x + 4, y + 3, name, -1, gr_string_mode);
                gr_set_color(200, 200, 200, 0xff);
                gr_string(x + 4, y + name_h + 3 + 3, overhead_label_slot_owner[side][row]->text, -1, gr_string_mode);
            } else {
                int x = (int)px;
                int y = (int)py;
                gr_set_color(0, 0, 0, 190);
                gr_rect(x, y, w + 8, h + 6, gr_rect_mode, 1.0f);
                gr_set_color(200, 200, 200, 0xff);
                gr_string(x + 4, y + 3, overhead_label_slot_owner[side][row]->text, -1, gr_string_mode);
            }
        }
    }
}

// 0x485380  Confidence: likely (the three texts share the projection code; the string width of the
// third goes to the dead argument's slot). Above living entities within 50 m: the HP change ("%d",
// green when healed; yellow for the party, red-brown on ionaext02, white otherwise), the stat change
// ("%d%%", blue, stacked above the first) and the character's own float text, each faded by its timer.
void levelrender_draw_float_text(levelrender_entry *e)
{
    char buf[0x14];
    vector3 p;
    gr_vertex v;
    int clip_x, clip_y, clip_w, clip_h;
    int w, h, hh;
    int x, y;
    bool drew_hp;

    if (!levelrender_show_float_text)
        return;
    Character *c = (Character *)e->object;
    if (c->type != 4 && c->type != 7 && c->type != 5 && c->type != 9 && c->type != 8 && c->type != 6
        && c->type != 10)
        return;
    if (fabs(cur_character->pos.y - c->pos.y) > 4.0f || distance_squared(&cur_character->pos, &c->pos) > 2500.0f)
        return;
    gr_get_clip(&clip_x, &clip_y, &clip_w, &clip_h);
    drew_hp = false;
    if (c->hp_text_timer != 0.0f) {
        sprintf(buf, "%d", (int)fabs(c->hp_overflow));
        p = e->object->pos;
        p.y = c->size_height() + p.y + 0.3f;
        gr_rotate_vertex(&v, &p);
        gr_project_vertex(&v);
        gr_get_string_size(&w, &h, buf, -1, -1);
        hh = h + 6;
        v.sy = v.sy - (float)hh;
        v.sx = v.sx - ((float)w * 0.5f + 4.0f) - clip_x;
        x = (int)v.sx + 4;
        y = (int)v.sy + 3;
        if (c->hp_overflow > 0.0f)
            gr_set_color(0, 0xff, 0, (int)(c->hp_text_timer * 127.0f));
        else if (((Character *)e->object)->type == 7)
            gr_set_color(0xff, 0xff, 0, (int)(c->hp_text_timer * 127.0f));
        else if (!_stricmp(level_filename, "ionaext02"))
            gr_set_color(200, 0, 0, (int)(c->hp_text_timer * 127.0f));
        else
            gr_set_color(0xff, 0xff, 0xff, (int)(c->hp_text_timer * 127.0f));
        gr_string(x, y, buf, -1, gr_string_mode);
        drew_hp = true;
    }
    if (c->pos_mod_text_timer != 0.0f) {
        sprintf(buf, "%d%%", (int)(c->pos_mod_text * 100.0f));
        p = e->object->pos;
        p.y = c->size_height() + p.y + 0.3f;
        gr_rotate_vertex(&v, &p);
        gr_project_vertex(&v);
        gr_get_string_size(&w, &h, buf, -1, -1);
        hh = h + 6;
        float fh = (float)hh;
        v.sy = v.sy - fh;
        if (drew_hp)
            v.sy = v.sy - fh;
        v.sx = v.sx - ((float)w * 0.5f + 4.0f) - clip_x;
        x = (int)v.sx + 4;
        y = (int)v.sy;
        gr_set_color(99, 139, 242, (int)(c->pos_mod_text_timer * 127.0f));
        gr_string(x, y, buf, -1, gr_string_mode);
    }
    if (strlen(c->float_text)) {
        p = e->object->pos;
        p.y = c->size_height() + p.y + 0.3f;
        gr_rotate_vertex(&v, &p);
        gr_project_vertex(&v);
        gr_get_string_size(&w, &h, c->float_text, -1, -1);
        v.sy = v.sy + 6.0f;
        v.sx = v.sx - ((float)w * 0.5f + 4.0f) - clip_x;
        x = (int)v.sx + 4;
        y = (int)v.sy + 3;
        gr_set_color(c->float_text_rgb[0], c->float_text_rgb[1], c->float_text_rgb[2],
                     (int)(c->float_text_scale * 127.0f));
        gr_string(x, y, c->float_text, -1, gr_string_mode);
    }
}

// 0x485810  Confidence: likely. Outside cutscenes and game flag 0x100: the quest icon over every visible
// living entity (not dead, not a monster or type 9) within 10 m that is an important speaker of the
// level's conversation file.
void levelrender_draw_speaker_icons()
{
    Character *c;

    if ((game_flags & 0x100) || (game_flags & 0x20))
        return;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->flags & CF_DEAD)
            continue;
        if (c->type == 5 || c->type == 9)
            continue;
        if (!(c->obj_flags & 2))
            continue;
        if (!conversation_db.has_speaker(c->get_name()))
            continue;
        if (!(conversation_db.find_speaker(c->get_name())->flags & CFLAG_IMPORTANT))
            continue;
        float dx = c->pos.x - cur_character->pos.x;
        float dy = c->pos.y - cur_character->pos.y;
        float dz = c->pos.z - cur_character->pos.z;
        if (dz * dz + dx * dx + dy * dy < 100.0f)
            character_draw_overhead_bitmap(c, cursoricon_quest_bm);
    }
}

// 0x4858f0  Confidence: likely (the scope objects of the case blocks sit where the EH state changes; the
// shared tail of cases 4 and 5 is the compiler's). One gr_light_desc for the whole list (reset per
// entry); items and containers each construct their own.
void levelrender_draw_list(int count, levelrender_entry **list, bool sorted)
{
    render_scope scope;
    gr_light_desc light;
    float saved_fog_near = 0.0f, saved_fog_far = 0.0f;
    float fog_limit = 0.0f;
    unsigned int num_lights;
    attached_light *lights[16];
    float weights[16];
    int i;

    for (i = 0; i < count; i++) {
        levelrender_entry *e = list[i];
        switch (e->type) {
        case LRE_LOBJ: {
            render_scope block;
            lobj *l = &lobjs[e->lobj];
            int alpha;
            int project;
            unsigned int k;

            if (l->flags & 0x80)
                break;
            alpha = (levelrender_lobj_states[e->lobj].flags >> LRS_ALPHA_SHIFT) & 0xff;
            if (Cull_ground) {
                if (Cull_ground_too)
                    break;
                if (!(l->flags & 0x40))
                    break;
            } else if (levelrender_fade_alpha < 0xff && (Cull_ground_too || !(l->flags & 0x40))) {
                alpha = levelrender_fade_alpha;
            }
            gr_set_color(0xff, 0xff, 0xff, 0xff);
            if (l->flags & 0x1000000) {
                fog_limit = 160.0f;
                if (!(game_flags & 0x20))
                    fog_limit = 80.0f;
                if (gr_fog_far > fog_limit) {
                    saved_fog_far = gr_fog_vertex_far;
                    saved_fog_near = gr_fog_vertex_near;
                    gr_fog_vertex_far = gr_scale_by_far_clip(fog_limit);
                    gr_fog_vertex_near = gr_fog_vertex_far - 1.0f / gr_fog_alpha_inv_range;
                }
            }
            project = 0;
            if ((l->flags & 0x100040) && e->dist < 20.0f)
                project = 1;
            gr_start_instance(&l->pos, &l->orient, project);
            num_lights = 0;
            if (!gr_tint_enabled && Summoner_options.cfg_21 && !(l->flags & 0x20001)) {
                attached_light **p = levelrender_level_lights;
                for (int n = levelrender_num_level_lights; n > 0; n--, p++) {
                    if (*p)
                        levelrender_rank_light(l, *p, &num_lights, weights, lights);
                }
            }
            for (k = 0; k < num_lights; k++) {
                attached_light *light2 = lights[k];
                light2->world_pos.x = light2->pos.x - l->pos.x;
                light2->world_pos.y = light2->pos.y - l->pos.y;
                light2->world_pos.z = light2->pos.z - l->pos.z;
                light2->world_pos.rotate(l->orient);
            }
            if (l == hover_tint_char)
                gr_set_tint(1, hover_tint_char_r, hover_tint_char_g, hover_tint_char_b);
            switch (gr_screen.mode) {
            case 0x68:
                levelrender_draw_list_opengl(l, alpha, num_lights, lights);
                break;
            case 0x67:
                levelrender_draw_list_glide(l, alpha, num_lights, lights);
                break;
            case 0x66:
                levelrender_draw_list_d3d(l, alpha, num_lights, lights);
                break;
            }
            if (project && levelrender_lobj_collect_projections(e->lobj) > 0)
                decals_draw_for_lobj(e->lobj, (l->flags & 0x20) ? levelrender_wave_vertices : l->mesh->vertices);
            gr_stop_instance();
            gr_set_tint(0, 0.0f, 0.0f, 0.0f);
            if ((l->flags & 0x1000000) && gr_fog_far > fog_limit) {
                gr_fog_vertex_far = saved_fog_far;
                gr_fog_vertex_near = saved_fog_near;
            }
            break;
        }
        case LRE_ATTACHMENT: {
            render_scope block;
            levelrender_attach_entry *a = (levelrender_attach_entry *)e;
            ubyte alpha;

            gr_set_color(0xff, 0xff, 0xff, 0xff);
            light.reset();
            if (a->character)
                alpha = a->character->get_move_fade_alpha();
            else
                alpha = 0xff;
            light.reset();
            light.f14 = a->light;
            light._unk08 = alpha;
            ca_render_set_mesh(a->item->mesh);
            ca_render_mesh(&a->pos, &a->orient, &light);
            ca_render_set_mesh(NULL);
            break;
        }
        case LRE_CHARACTER: {
            render_scope block;
            Character *c = (Character *)e->object;
            vfx_instance *vi = NULL;
            bool has_fx;
            ubyte fade;

            if (c->info && (c->info->flags & CIF_NO_DRAW))
                break;
            gr_set_color(0xff, 0xff, 0xff, 0xff);
            light.reset();
            if (c && c == world_hover_object)
                levelrender_hover_target = e;
            if (c->type == 7 && !(c->flags & CF_SUMMON)) {
                Player *pl = player_from_handle(e->object->handle);
                gr_vertex v;
                gr_rotate_vertex(&v, &e->object->pos);
                gr_project_vertex(&v);
                pl->_unk18 = (int)v.sx;
                pl->_unk1c = (int)v.sy;
            }
            if (c == levelrender_fx_char)
                gr_set_tint(1, levelrender_fx_char_tint_r, levelrender_fx_char_tint_g, levelrender_fx_char_tint_b);
            if (e->object == hover_tint_obj)
                gr_set_tint(1, hover_tint_obj_r, hover_tint_obj_g, hover_tint_obj_b);
            if (c->info->flags & CIF_EDGED)
                light._unk00 |= 4;
            light.f14 = c->light_level;
            has_fx = false;
            if (!(game_flags & 0x20) || (c->flags & 0x800)) {
                if (c->flags & 0x200000) {
                    for (fx_char_link *link = fx_char_link_used.next; link != &fx_char_link_used; link = link->next) {
                        if (link->character == c) {
                            has_fx = true;
                            vi = link->fx->instance;
                            break;
                        }
                    }
                }
            }
            fade = 0xff;
            if (has_fx)
                vfx_instance__setup_render_state(&fade, vi);
            if (c->info && c->info->alt_parent)
                light.appearance = (char *)c->info->alt_textures + 0x2c;
            if (c->info->flags & CIF_MULTI_SLOT)
                light.appearance = charinfo_get_multi_appearance(c->info);
            light._unk04 = -1;
            if (!(game_flags & 0x120)) {
                int detail = Summoner_options.detail_18;
                if (c->type == 7)
                    detail = Summoner_options.detail_14;
                if (detail == 0) {
                    float limit;
                    if (c->type == 7)
                        limit = Main_character_lod_dist * 0.8f;
                    else
                        limit = Character_lod_dist * 0.7f;
                    if (limit > e->depth)
                        light._unk04 = 0;
                    else
                        light._unk04 = 2;
                } else if (detail == 1) {
                    light._unk04 = 0;
                } else {
                    float limit = c->type == 7 ? Main_character_lod_dist : Character_lod_dist;
                    light._unk04 = 2;
                    if (limit > e->depth)
                        light._unk04 = 0;
                }
            } else {
                light._unk04 = 0;
            }
            if (levelrender_no_lighting)
                light._unk04 = 2;
            if (c->instance->_unkb4 != 0xff) {
                light._unk08 = (fade * c->instance->_unkb4) / 255 & 0xff;
            } else if (!char_fog_enabled) {
                light._unk08 = fade;
            } else {
                vector3 *op = &e->object->pos;
                float dx = camera_view_pos.x - op->x;
                float dy = camera_view_pos.y - op->y;
                float dz = camera_view_pos.z - op->z;
                float dist_sq = dx * dx + dy * dy + dz * dz;
                if (dist_sq > char_fog_near_sq) {
                    if (dist_sq > char_fog_far_sq) {
                        c->obj_flags &= ~2;
                        break;
                    }
                    light._unk08 = (int)((char_fog_far_sq - dist_sq) / (char_fog_far_sq - char_fog_near_sq) * 255.0f);
                }
            }
            light.model_prefix = c->hero_model_prefix();
            c->instance->render(&c->pos, &c->orient, &light);
            if (has_fx)
                vfx_render_state_reset();
            levelrender_fx_char = NULL;
            gr_set_tint(0, 0.0f, 0.0f, 0.0f);
            bool visible = !(game_flags & 0x20) && (c->obj_flags & 2);
            if (c->instance->_unkb4 > 0) {
                int n = levelrender_num_float_texts;
                if (visible) {
                    if (c->hp_text_timer > 0.0f) {
                        if (n < LEVELRENDER_MAX_FLOAT_TEXTS)
                            levelrender_float_texts[n++] = e;
                    } else if (c->pos_mod_text_timer > 0.0f) {
                        if (n < LEVELRENDER_MAX_FLOAT_TEXTS)
                            levelrender_float_texts[n++] = e;
                    } else if (c->float_text_scale > 0.0f && n < LEVELRENDER_MAX_FLOAT_TEXTS) {
                        levelrender_float_texts[n++] = e;
                    }
                }
                levelrender_num_float_texts = n;
            }
            break;
        }
        case LRE_EFFECT: {
            render_scope block;
            levelrender_effect_entry *fe = (levelrender_effect_entry *)e;

            gr_set_color(0xff, 0xff, 0xff, 0xff);
            if ((fe->fx->bits & 2) && Cull_ground)
                break;
            if (fe->fx->instance && (fe->fx->instance->id_bits & 0x100))
                levelrender_fog_disable();
            if (fe->fx->bits & 8) {
                gr_set_texture(fe->fx->trail_bitmap, -1);
                gr_line_quad_3d(&fe->fx->start, &fe->fx->pos, fe->fx->radius, gr_mode_line_quad);
            } else {
                fe->fx->instance->render_alpha = (ubyte)(fe->fx->bits >> 24);
                fe->fx->instance->render_node(&fe->ref, &fe->fx->pos, &fe->fx->orient);
            }
            if (fe->fx->instance && (fe->fx->instance->id_bits & 0x100))
                levelrender_fog_enable();
            break;
        }
        case LRE_ITEM: {
            gr_set_color(0xff, 0xff, 0xff, 0xff);
            item_obj *it = (item_obj *)e->object;
            gr_light_desc item_light;
            item_light.reset();
            if (e->object == hover_tint_obj)
                gr_set_tint(1, hover_tint_obj_r, hover_tint_obj_g, hover_tint_obj_b);
            ca_render_set_mesh(it->item->type->mesh);
            ca_render_mesh(&it->pos, &it->orient, &item_light);
            ca_render_set_mesh(NULL);
            gr_set_tint(0, 0.0f, 0.0f, 0.0f);
            break;
        }
        case LRE_CONTAINER: {
            gr_set_color(0xff, 0xff, 0xff, 0xff);
            container *ct = (container *)e->object;
            gr_light_desc box_light;
            box_light.reset();
            if (e->object == hover_tint_obj)
                gr_set_tint(1, hover_tint_obj_r, hover_tint_obj_g, hover_tint_obj_b);
            ca_render_set_mesh(item_types[ct->mesh_item_type].mesh);
            ca_render_mesh(&ct->pos, &ct->orient, &box_light);
            ca_render_set_mesh(NULL);
            gr_set_tint(0, 0.0f, 0.0f, 0.0f);
            break;
        }
        }
    }
}

// 0x4863b0  Confidence: likely (the shift loop). (r + g + b) * intensity, scaled down linearly between
// the two ranges by the distance from the light to the object's sphere; inserted into the 16 strongest.
void levelrender_rank_light(lobj *l, attached_light *light, unsigned int *count, float *weights,
                            attached_light **lights)
{
    float radius = l->mesh->_unk30;
    float dist;
    float w;
    unsigned int i, j;

    if ((float)fabs((l->centre - light->pos).len()) - radius < 0.0f)
        dist = 0.0f;
    else
        dist = (float)fabs((l->centre - light->pos).len()) - radius;
    if (light->has_range && dist > light->range_far)
        return;
    w = (light->b + light->g + light->r) * light->intensity;
    if (light->has_range && dist > light->range_near)
        w *= (light->range_far - dist) / (light->range_far - light->range_near);
    for (i = 0; i < *count; i++) {
        if (w > weights[i]) {
            if (*count < 16)
                (*count)++;
            for (j = 15; j > i; j--) {
                weights[j] = weights[j - 1];
                lights[j] = lights[j - 1];
            }
            weights[i] = w;
            lights[i] = light;
            return;
        }
    }
    if (*count < 16) {
        weights[*count] = w;
        lights[*count] = light;
        (*count)++;
    }
}

// 0x486520  Confidence: exact. The blob shadows (unless the object has flag 0x400) and the decals that
// touch the object, projected into its frame; returns the decal count (the shadows are counted in).
unsigned int levelrender_lobj_collect_projections(int index)
{
    projector_record *recs[0x40];
    int count = 0;
    lobj *l = &lobjs[index];

    if (!(l->flags & 0x400))
        shadow_collect_for_lobj(l, (shadow_record **)recs, &count);
    unsigned int n = decals_collect_for_lobj(l, recs, &count);
    lobj_project_records(index, recs, count);
    return n;
}

// ---- level load and unload ----------------------------------------------------------------------------
// 0x4865a0  Confidence: likely (returns 0, D4300; the order of the stores between the memsets). The
// level's detail_levels.tbl record, decals, face marks, the low camera fog defaults, per-layer snow on
// and fog cleared, the D3D vertex buffer, the lightmap detail flag, the render records (in the level
// mempool, their bytes counted in levelrender_lobj_state_bytes), the "$seethru" sets, the level lights,
// and a lamp glow for every lamp object (flag 8).
int levelrender_level_load()
{
    char name[0x20];
    int i, j;
    unsigned int before;

    if (num_detail_levels == 0)
        detail_levels_parse_table();
    file_strip_ext(name, level_s3d.filename);
    detail_level_entry_index = -1;
    for (i = 0; i < num_detail_levels; i++) {
        if (!_stricmp(name, detail_levels[i].name)) {
            detail_level_entry_index = i;
            options_apply_detail();
            break;
        }
    }
    decals_reset();
    lobj_face_marks_init();
    fog_set_low_camera_default();
    memset(level_layer_snow, 1, sizeof(level_layer_snow));
    levelrender_22fb6f6 = false;
    fog_color_r = 0;
    fog_color_g = 0;
    memset(level_layer_fog_near, 0, sizeof(level_layer_fog_near));
    memset(level_layer_fog_far, 0, sizeof(level_layer_fog_far));
    fog_color_b = 0;
    levelrender_create_vertex_buffer();
    levelrender_apply_lobj_detail_flag();
    if (!levelrender_lights_initialized) {
        levelrender_lights_initialized = true;
        for (i = 0; i < 16; i++)
            levelrender_lights[i]._unk55[0] = 0;
    }
    levelrender_clear_draw_lists();
    levelrender_num_level_lights = 0;
    levelrender_lobj_state_bytes = 0;
    set_current_mempool(&level_mempool);
    before = level_mempool.offset;
    levelrender_lobj_states = new lobj_render_state[num_lobjs];
    set_current_mempool(NULL);
    levelrender_lobj_state_bytes += level_mempool.offset - before;
    for (i = 0; i < num_lobjs; i++) {
        levelrender_lobj_states[i].flags &= ~(LRS_FADE_HIDDEN | LRS_FADE_MOVING);
        levelrender_lobj_states[i].flags &= ~LRS_OCCLUDING;
        levelrender_lobj_states[i].flags &= ~LRS_WAS_OCCLUDING;
        levelrender_lobj_states[i].flags |= LRS_ALPHA_MASK;
        levelrender_lobj_states[i].seethru = NULL;
        levelrender_lobj_states[i].timer.SetTimeout(0);
    }
    for (i = 0; i < level_num_vis_records; i++) {
        if (_strnicmp("$seethru", level_vis_records[i].name, 8))
            continue;
        for (j = 0; j < level_vis_records[i].count; j++)
            levelrender_lobj_states[level_vis_records[i].indices[j]].seethru = &level_vis_records[i];
    }
    level_lights_setup();
    set_current_mempool(&level_mempool);
    before = level_mempool.offset;
    levelrender_lobj_glow_map = new lobj_glow_ref[num_lobjs];
    levelrender_lobj_state_bytes += level_mempool.offset - before;
    set_current_mempool(NULL);
    for (i = 0; i < num_lobjs; i++) {
        levelrender_lobj_glow_map[i].occluder = -1;
        if (lobjs[i].flags & 8)
            levelrender_lobj_glow_map[i].glow = (short)lamp_glow_add(&lobjs[i].centre, lobjs[i].mesh->_unk30);
    }
    return 0;
}

// 0x4868f0  Confidence: likely (the if / else shape of the flag stores). Up to 41 records of
// "name" fog_multiplier [+UnlockMedium | +UnlockHigh] [+NoSmallMeshCulling].
void detail_levels_parse_table()
{
    TextFile tf;

    num_detail_levels = 0;
    if (tf.Open("detail_levels.tbl", PATH_ANY))
        return;
    while (num_detail_levels < MAX_DETAIL_LEVELS && tf.Peek("\"")) {
        detail_level *d = &detail_levels[num_detail_levels];
        tf.ParseString(d->name, 0x1e, '"', '"');
        d->fog_multiplier = tf.ParseFloat();
        if (tf.Accept("+UnlockMedium")) {
            d->unlock_medium = true;
            d->unlock_high = true;
        } else if (tf.Accept("+UnlockHigh")) {
            d->unlock_medium = false;
            d->unlock_high = true;
        } else {
            d->unlock_medium = false;
            d->unlock_high = false;
        }
        if (tf.Accept("+NoSmallMeshCulling"))
            d->small_mesh_culling = false;
        else
            d->small_mesh_culling = true;
        num_detail_levels++;
    }
}

// 0x486a40  Confidence: exact. Lightmaps off (Summoner_options.lightmaps_off) sets lobj flag 0x80000.
void levelrender_apply_lobj_detail_flag()
{
    int i;

    if (!Summoner_options.lightmaps_off) {
        for (i = 0; i < num_lobjs; i++)
            lobjs[i].flags &= ~0x80000;
    } else {
        for (i = 0; i < num_lobjs; i++)
            lobjs[i].flags |= 0x80000;
    }
}

// 0x486ab0  Confidence: exact. The render records live in the level mempool and are not freed.
void levelrender_level_unload()
{
    fog_level_near_distance = 35.0f;
    fog_level_far_distance = 50.0f;
    fog_level_default_set = false;
    levelrender_attach_count = 0;
    levelrender_sort_count = 0;
    lamp_glow_forget();
    levelrender_lobj_states = NULL;
    levelrender_free_d3d_vertex_buffer();
    game_camera.high_view_suppressed = 0;
}

// 0x486b00  Confidence: exact.
void levelrender_clear_draw_lists()
{
    levelrender_attach_count = 0;
    levelrender_sort_count = 0;
}

// ---- fog ----------------------------------------------------------------------------------------------
// 0x486b10  Confidence: exact. A value <= 0 takes the level default; far stays at least 10 m beyond near.
void fog_set_distances(float near_dist, float far_dist)
{
    if (far_dist > 0.0f)
        fog_far_distance = far_dist;
    else
        fog_far_distance = fog_level_far_distance;
    if (near_dist > 0.0f)
        fog_near_distance = near_dist;
    else
        fog_near_distance = fog_level_near_distance;
    if (!(fog_far_distance > fog_near_distance + 10.0f))
        fog_far_distance = fog_near_distance + 10.0f;
}

// 0x486b90  Confidence: exact. Once per level (until levelrender_level_unload).
void fog_set_level_default(float a, float b)
{
    if (!fog_level_default_set) {
        fog_level_near_distance = a;
        fog_level_far_distance = b;
        fog_level_default_set = true;
    }
}

// 0x486bc0  Confidence: exact.
void fog_restore_level_default()
{
    fog_near_distance = fog_level_near_distance;
    fog_far_distance = fog_level_far_distance;
}

// 0x486be0  Confidence: exact.
bool levelrender_get_flag_22fb6f7()
{
    return levelrender_flag_22fb6f7;
}

// 0x486bf0  Confidence: exact.
float fog_get_near_current()
{
    if (fog_fading)
        return (1.0f - fog_fade_amount) * fog_near_distance;
    return fog_near_distance;
}

// 0x486c20  Confidence: exact.
float fog_get_far_current()
{
    if (fog_fading) {
        float far_dist = (1.0f - fog_fade_amount) * fog_far_distance;
        if (15.0f > far_dist)
            return 15.0f;
        return far_dist;
    }
    return fog_far_distance;
}

// ---- the .vis file ------------------------------------------------------------------------------------
// 0x486c60  Confidence: likely (the TextFile scope). "<level>.vis" from data\internal; without one every
// layer sees the $vis_base set. Always 0.
int level_vis_load(const char *level_filename)
{
    char name[0x101];

    level_vis_cache_key[0] = 0;
    level_vis_cache_key[1] = 0;
    level_rendered_layer = -1;
    strncpy(name, level_filename, 0x100);
    strncat(name, ".vis", 0x100);
    name[0x100] = 0;
    {
        TextFile tf;
        if (tf.Open(name, path_internal)) {
            level_vis_reset();
        } else {
            level_vis_parse(&tf);
            tf.Close();
        }
    }
    return 0;
}

// 0x486d40  Confidence: likely (the "$layer" test inside the loop re-reads the outer name, as the code
// does). "$vis_set" "$layerN" [$high_camera | $low_camera] then quoted set names until the next
// "$vis_set" or "$end". Each layer gets its sets ($vis_base first) and their "HIGH" variants (the set
// itself when there is none); layers without sets get $vis_base. A name that is not "$layer1".."$layer20"
// stops the parse.
void level_vis_parse(TextFile *tf)
{
    char set_name[0x40];
    char layer_name[0x40];
    char line[0x80];
    char high_name[0x44];
    int i;

    for (i = 0; i < LEVEL_NUM_LAYERS; i++) {
        level_layer_vis[i].count = 0;
        level_layer_vis[i].sets = level_vis_set_lists;
        level_layer_vis[i].sets_high = level_vis_set_records;
    }
    level_num_vis_sets = 0;
    while (tf->Accept("$vis_set")) {
        tf->ParseString(layer_name, 0x40, '"', '"');
        if (_strnicmp(layer_name, "$layer", 6))
            return;
        int layer = atoi(layer_name + 6);
        if (layer == 0 || (unsigned int)layer > LEVEL_NUM_LAYERS)
            return;
        if (level_num_vis_sets >= LEVEL_MAX_VIS_SETS)
            return;
        layer--;
        if (tf->Accept("$high_camera")) {
            level_layer_vis_flags[layer] = 1;
        } else {
            tf->Accept("$low_camera");
            level_layer_vis_flags[layer] = 0;
        }
        level_layer_vis_rec *rec = &level_layer_vis[layer];
        rec->count = 1;
        rec->sets = &level_vis_set_lists[level_num_vis_sets];
        level_vis_set_lists[level_num_vis_sets] = (s3d_vis_set *)&level_vis_base_name;
        rec->sets_high = &level_vis_set_records[level_num_vis_sets];
        level_vis_set_records[level_num_vis_sets] = (s3d_vis_set *)&level_vis_base_name;
        level_num_vis_sets++;
        while (tf->Peek("\"")) {
            tf->ParseString(set_name, 0x40, '"', '"');
            if (_strnicmp(layer_name, "$layer", 6))
                return;
            s3d_vis_set *set = level_vis_record_find(set_name);
            if (set == NULL) {
                sprintf(line, "could not find visibility selset %s, ignoring!\n", set_name);
                print_to_console(line, 0);
                continue;
            }
            strncpy(high_name, set_name, 4);
            strcpy(high_name + 4, "HIGH");
            strcat(high_name, set_name + 4);
            s3d_vis_set *high = level_vis_record_find(high_name);
            if (high == NULL)
                high = set;
            if (level_num_vis_sets >= LEVEL_MAX_VIS_SETS)
                return;
            level_vis_set_records[level_num_vis_sets] = high;
            level_vis_set_lists[level_num_vis_sets] = set;
            level_num_vis_sets++;
            rec->count++;
        }
    }
    tf->Expect("$end");
    if (level_num_vis_sets == 0) {
        level_num_vis_sets = 1;
        level_vis_set_lists[0] = (s3d_vis_set *)&level_vis_base_name;
        level_vis_set_records[0] = (s3d_vis_set *)&level_vis_base_name;
    }
    for (i = 0; i < LEVEL_NUM_LAYERS; i++) {
        if (level_layer_vis[i].count == 0) {
            level_layer_vis[i].count = 1;
            level_layer_vis[i].sets = level_vis_set_lists;
            level_layer_vis[i].sets_high = level_vis_set_records;
        }
    }
}

// 0x487030  Confidence: exact. Both lists of every layer point at the one $vis_base entry.
void level_vis_reset()
{
    int i;

    memset(level_layer_vis_flags, 0, sizeof(level_layer_vis_flags));
    level_num_vis_sets = 1;
    level_vis_set_lists[0] = (s3d_vis_set *)&level_vis_base_name;
    for (i = 0; i < LEVEL_NUM_LAYERS; i++) {
        level_layer_vis[i].count = 1;
        level_layer_vis[i].sets = level_vis_set_lists;
        level_layer_vis[i].sets_high = level_vis_set_lists;
    }
}

// 0x487080  Confidence: exact. The high camera's fog (35 / 50) when the layer has none of its own.
void fog_set_high_camera_default()
{
    if (!(level_layer_fog_far[level_current_layer] > 0.0f)) {
        fog_near_distance = 35.0f;
        fog_far_distance = 50.0f;
    }
}

// 0x4870b0  Confidence: exact. The low camera's (20 / 30).
void fog_set_low_camera_default()
{
    if (!(level_layer_fog_far[level_current_layer] > 0.0f)) {
        fog_near_distance = 20.0f;
        fog_far_distance = 30.0f;
    }
}

// 0x4870e0  Confidence: exact.
void fog_set_color(int r, int g, int b)
{
    fog_color_r = r;
    fog_color_g = g;
    fog_color_b = b;
}

// 0x487100  Confidence: exact (the colour is stored as bytes).
void fog_set_layer(int layer, float a, float b, int r, int g, int bl)
{
    if (layer >= 0 && layer < LEVEL_NUM_LAYERS) {
        level_layer_fog_far[layer] = a;
        level_layer_fog_r[layer] = (ubyte)r;
        level_layer_fog_near[layer] = b;
        level_layer_fog_g[layer] = (ubyte)g;
        level_layer_fog_b[layer] = (ubyte)bl;
    }
}

// 0x487150  Confidence: exact (the tail jump is level_vis_invalidate).
void level_vis_force_begin()
{
    level_vis_force_list[0] = (s3d_vis_set *)&level_vis_base_name;
    level_vis_forced = true;
    level_vis_force_count = 1;
    level_vis_invalidate();
}

// 0x487170  Confidence: exact.
void level_vis_invalidate()
{
    level_rendered_layer = -1;
    level_vis_cache_key[0] = 0;
    level_vis_cache_key[1] = 0;
    level_vis_dirty = true;
}

// 0x487190  Confidence: exact.
void level_vis_force_reset()
{
    level_vis_force_count = 1;
    level_vis_force_list[0] = (s3d_vis_set *)&level_vis_base_name;
    level_vis_invalidate();
}

// 0x4871b0  Confidence: exact.
void level_vis_force_add(const char *name)
{
    if (level_vis_force_count < LEVEL_MAX_FORCED_VIS) {
        s3d_vis_set *set = level_vis_record_find(name);
        if (set)
            level_vis_force_list[level_vis_force_count++] = set;
    }
}

// 0x4871e0  Confidence: exact.
void level_vis_force_end()
{
    level_vis_forced = false;
    level_vis_invalidate();
}

// 0x4871f0  Confidence: exact.
void fog_layer_set_snow(int layer, bool snow)
{
    if (layer >= 0 && layer < LEVEL_NUM_LAYERS)
        level_layer_snow[layer] = snow;
}

// 0x487210  Confidence: exact.
bool fog_layer_snow_allowed(int layer)
{
    if (layer >= 0 && layer < LEVEL_NUM_LAYERS)
        return level_layer_snow[layer];
    return false;
}

// 0x487230  Confidence: likely (effect_get_base is called per flagged emitter). The particles of the
// emitters levelrender_add_effects updated this frame.
void fx_draw_flagged_particles()
{
    effect_object *fx;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->instance == NULL)
            continue;
        vfx_inst21c *em = fx->instance->insts21c;
        if (em == NULL)
            continue;
        int k = 0;
        do {
            if (em->flags & 0x10) {
                vif_draw_particles(em, &effect_get_base(fx->instance->id_bits >> 9)->descs21c[k]);
                em->flags &= ~0x10;
            }
            em = em->next;
            k++;
        } while (em != fx->instance->insts21c && em);
    }
}

// 0x4872b0  Confidence: exact.
void overhead_label_slots_clear()
{
    memset(overhead_label_slot_owner, 0, sizeof(overhead_label_slot_owner));
}

// ---- detail options -----------------------------------------------------------------------------------
// 0x4872d0  Confidence: exact. Texture quality 0 halves the textures; detail 2 / 1 / 0 allows 6 / 4 / 2
// lights per object.
void options_apply_detail()
{
    bm_set_reduce_textures(Summoner_options.texture_quality == 0);
    lights_set_detail_flag(Summoner_options.detail_24 == 2);
    lights_set_dynamic_enabled(Summoner_options.cfg_21 != 0);
    if (Summoner_options.detail_24 == 2)
        lights_set_max_per_object(6);
    else if (Summoner_options.detail_24 == 1)
        lights_set_max_per_object(4);
    else
        lights_set_max_per_object(2);
    levelrender_apply_lobj_detail_flag();
    detail_apply_level_settings();
    gr_set_antialias(Summoner_options.antialias != 0);
    gr_set_gamma(Summoner_options.gamma_correction);
    gr_vsync = Summoner_options.cfg_34 != 0;
}

// 0x487370  Confidence: likely (the order of the Small_mesh_threshold stores). The level's record: fog
// quality 1 halves the fog multiplier's distance from 1.0 and unlocks the high camera with +UnlockMedium,
// quality 2 uses the multiplier and +UnlockHigh; quality 0 or no record: multiplier 1.0 and no high
// camera. A pending high-camera request is honoured when it is allowed again. S3 cards keep 1.0.
void detail_apply_level_settings()
{
    detail_level *d = &detail_levels[detail_level_entry_index];
    bool suppressed;

    if (detail_level_entry_index >= 0 && Summoner_options.fog_quality != 0) {
        if (Summoner_options.fog_quality == 1) {
            Gr_level_fog_multiplier = (d->fog_multiplier - 1.0f) * 0.5f + 1.0f;
            suppressed = d->unlock_medium != 0;
        } else {
            Gr_level_fog_multiplier = d->fog_multiplier;
            suppressed = d->unlock_high != 0;
        }
    } else {
        Gr_level_fog_multiplier = 1.0f;
        Small_mesh_threshold = 3.0f;
        suppressed = false;
    }
    game_camera.high_view_suppressed = suppressed;
    if (detail_level_entry_index >= 0) {
        Small_mesh_threshold = 3.0f;
        if (!d->small_mesh_culling)
            Small_mesh_threshold = 0.0f;
    }
    if (!suppressed && game_camera.high_view_requested)
        game_camera.set_mode(3);
    if (d3d_s3_depth)
        Gr_level_fog_multiplier = 1.0f;
}
