// engine/rendering/unnamed_shadow_table_0x487440.h - shadows.tbl, the character blob shadows of the draw
// lists, the move-target decal and the ground decals: the unit 0x487440-0x48789f (source
// game/unnamed_shadow_table_0x487440.cpp, decomp63). File and directory names are placeholders: no path
// string, no strings.tbl block. shadows_parse_table, move_target_set_bitmap / move_target_draw_decal and
// place_ground_decal stay declared in levelrender.h. Included at the end of levelrender.h.
#ifndef ENGINE_RENDERING_UNNAMED_SHADOW_TABLE_0X487440_H
#define ENGINE_RENDERING_UNNAMED_SHADOW_TABLE_0X487440_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "engine/rendering/levelrender.h"

class Character;

#define MAX_SHADOW_ENTRIES 0xaf       // shadows.tbl records kept (shadows_parse_table)

// One "$Object:" record of shadows.tbl (0x50 bytes; names ours). Nothing in Sum.exe reads the table back:
// shadows_draw_characters passes 20.0 for every character (see NOTES).
struct shadow_entry {
    char name[0x40];                  // +0x00 ParseString(.., 0x40, '"', '"')
    int _unk40;                       // +0x40 never written (the stack record is copied whole)
    float radius;                     // +0x44 "$Radius:"
    float scale;                      // +0x48 "$Scale:"
    ubyte flags;                      // +0x4c bit 0 "+Prop:" (cleared as a byte)
    ubyte _pad4d[3];
};
LAYOUT_CHECK(shadow_entry, sizeof(shadow_entry) == 0x50);

// ---- .data (0x59c120-0x59c137, the literals follow to 0x59c17f) -----------------------------------------
// shadows_enabled (0x59c120 = 1) is declared in levelrender.h.
extern int move_target_bm;            // 0x59c124 -1
extern float shadow_anchor_height;    // 0x59c128 2.0 above the root bone (name ours)
extern float shadow_fov;              // 0x59c12c 0.15 shadow_record_add's third argument (name ours)
extern float shadow_near;             // 0x59c130 1.0 (name ours)
extern float shadow_far;              // 0x59c134 5.0 (name ours)
// ---- .bss (0x22fb720-0x22fedd3) ------------------------------------------------------------------------
extern shadow_entry shadow_entries[MAX_SHADOW_ENTRIES];   // 0x22fb720 (name ours)
extern int shadows_num_entries;       // 0x22fedd0

// shadow_bm (0x59c25c, bm_load("Shadow01.tga") in shadows_init) belongs to the fx unit's .data.
extern int shadow_bm;                 // 0x59c25c

// (character, out position, out orientation): the root bone position plus 2.0 up and an identity
// orientation with the uvec negated (the projection looks down).
void shadow_get_character_anchor(Character *c, vector3 *pos, matrix *orient);   // 0x487440
// (character, radius, offset): the last two are not read (shadows_draw_characters passes 20.0 and
// &vector3_zero; shadow_bm and the .data floats are used).
void shadow_draw_character(Character *c, float radius, const vector3 *offset);   // 0x487630
// (eye position, eye orientation, count, list): both eye arguments are unused.
void shadows_draw_characters(const vector3 *eye_pos, const matrix *eye_orient, int count,
                             levelrender_entry **list);   // 0x4876b0

#endif
