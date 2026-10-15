// unnamed_shadow_table_0x487440.cpp - Sum.exe v1.40, code 0x487440-0x48789f: shadows.tbl, the blob
// shadows of the living entities in the draw lists, the move-target decal and the ground decals.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (tools/asm.mjs, llvm-objdump). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 /Ob1. Confidence per function: exact (control flow, calls, constants and globals match), likely (same,
// but one detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range (decomp63, NOTES.md): .data 0x59c120-0x59c17f starts with the variable shadows_enabled (= 1)
// right after the render-scene unit's last literal ("$vis_set" 0x59c114) and ends with the shadows.tbl
// literals ("+Prop:" 0x59c138 .. "shadows.tbl" 0x59c174); multichat.cpp's .data starts at 0x59c180 with
// its command table. .bss 0x22fb720-0x22fedd3 (the 0xaf records and their count; multichat.cpp's ring
// starts at 0x22fedd8). The first function, shadow_get_character_anchor (0x487440), is the first to use
// this .data (0x59c128); the render-scene unit's last function (0x487370) uses only that unit's. The end
// of the unit is not marked: the three equip packet functions after place_ground_decal (0x4878a0..)
// touch no .data or .bss of their own; they are kept in their own placeholder file (see NOTES).
// "#End" (0x58bd08) is a pooled copy of another unit's literal.

#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/rendering/levelrender.h"
#include "engine/rendering/unnamed_projectors_0x4c0250.h"
#include "engine/rendering/unnamed_shadows_0x4a37d0.h"
#include "engine/rendering/unnamed_decals_0x4bbe80.h"
#include "summoner/player/player.h"

#define SHADOW_DEG_TO_RAD 0.017453f   // 0x57ef38 (shared __real@ constant)

// ---- .data (0x59c120-0x59c137) ------------------------------------------------------------------------
bool shadows_enabled = true;          // 0x59c120 (read by shadows_render too)
int move_target_bm = -1;              // 0x59c124
float shadow_anchor_height = 2.0f;    // 0x59c128 (name ours)
float shadow_fov = 0.15f;             // 0x59c12c (name ours)
float shadow_near = 1.0f;             // 0x59c130 (name ours)
float shadow_far = 5.0f;              // 0x59c134 (name ours)

// ---- .bss (0x22fb720-0x22fedd3) -----------------------------------------------------------------------
shadow_entry shadow_entries[MAX_SHADOW_ENTRIES];   // 0x22fb720 (name ours)
int shadows_num_entries;              // 0x22fedd0

// 0x487440  Confidence: likely (the order of the position statements). The root bone of the current
// mesh, an identity orientation whose uvec points down.
void shadow_get_character_anchor(Character *c, vector3 *pos, matrix *orient)
{
    c->instance->get_bone_transform(pos, orient, c->instance->character->mesh->skin->root_bone);
    orient->make_identity();
    orient->uvec.x *= -1.0f;
    orient->uvec.y *= -1.0f;
    orient->uvec.z *= -1.0f;
    pos->x = c->pos.x + pos->x;
    pos->y = c->pos.y + pos->y;
    pos->z = c->pos.z + pos->z;
    pos->y = shadow_anchor_height + pos->y;
}

// 0x4874c0  Confidence: likely (where the flags byte is cleared). "#Object Shadow info", then records
// "$Object:" "name" "$Radius:" r "$Scale:" s ["+Prop:"] up to "#End"; at most 0xaf are kept. Nothing reads
// the table back (shadow_entries is referenced only here).
void shadows_parse_table()
{
    TextFile tf;
    shadow_entry entry;

    shadows_num_entries = 0;
    if (tf.Open("shadows.tbl", PATH_ANY))
        return;
    tf.Expect("#Object Shadow info");
    while (!tf.Accept("#End")) {
        tf.Expect("$Object:");
        tf.ParseString(entry.name, 0x40, '"', '"');
        entry.flags = 0;
        tf.Expect("$Radius:");
        entry.radius = tf.ParseFloat();
        tf.Expect("$Scale:");
        entry.scale = tf.ParseFloat();
        if (tf.Accept("+Prop:"))
            entry.flags |= 1;
        if (shadows_num_entries < MAX_SHADOW_ENTRIES)
            shadow_entries[shadows_num_entries++] = entry;
    }
    tf.Close();
}

// 0x487630  Confidence: exact. Species 0xcd / 0xce / 9 cast no shadow in AI mode 6.
void shadow_draw_character(Character *c, float radius, const vector3 *offset)
{
    vector3 pos;
    matrix orient;

    if (c->info->species == 0xcd || c->info->species == 0xce || c->info->species == 9) {
        if (c->ai.is_mode(6, -1))
            return;
    }
    shadow_get_character_anchor(c, &pos, &orient);
    shadow_record_add(&pos, &orient, shadow_fov, shadow_near, shadow_far, shadow_bm, NULL, false);
}

// 0x4876b0  Confidence: exact. Living-entity entries within a sort key of 20 m, party members, NPCs and
// monsters (types 7, 4, 5) whose char_info has no +No Shadow, from the end of the list.
void shadows_draw_characters(const vector3 *eye_pos, const matrix *eye_orient, int count,
                             levelrender_entry **list)
{
    int i;

    if (!shadows_enabled)
        return;
    for (i = count - 1; i >= 0; i--) {
        levelrender_entry *e = list[i];
        Character *c = (Character *)e->object;
        if (e->type != LRE_CHARACTER || e->dist > 20.0f)
            continue;
        if (c->type != 7 && c->type != 4 && c->type != 5)
            continue;
        int info = charinfo_find_by_name(c->instance->character->name);
        if (info >= 0 && (char_infos[info].flags & CIF_NO_SHADOW))
            continue;
        shadow_draw_character(c, 20.0f, &vector3_zero);
    }
}

// 0x487750  Confidence: exact.
void move_target_set_bitmap(int bm)
{
    move_target_bm = bm;
}

// 0x487760  Confidence: likely (the two axis vectors are built after the pushes, as inline constructors).
// While cur_character walks to a clicked point (goal priority 999999), a one-frame 1 x 1 m decal at the
// path's destination.
void move_target_draw_decal()
{
    vector3 dest;

    if (!cur_character->ai.is_traveling_sub0())
        return;
    if (cur_character->ai.goals[0].priority != 999999)
        return;
    if (!cur_character->ai.path_get_destination(&dest))
        return;
    vector3 v(0.0f, 0.0f, 0.5f);
    vector3 u(0.5f, 0.0f, 0.0f);
    decal_place(move_target_bm, &dest, &u, &v, 0, false, 0, 0.75f);
}

// 0x487800  Confidence: likely (the inline vector products). A one-frame decal turned by -yaw degrees:
// u = fvec * w, v = rvec * h.
void place_ground_decal(int bm, vector3 *pos, float yaw, float w, float h)
{
    matrix m;

    m.make_identity();
    m.set_from_angles(0.0f, 0.0f, -yaw * SHADOW_DEG_TO_RAD);
    vector3 v = m.rvec * h;
    vector3 u = m.fvec * w;
    decal_place(bm, pos, &u, &v, 0, false, 0, 0.75f);
}
