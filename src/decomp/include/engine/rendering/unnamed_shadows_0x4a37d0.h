// engine/rendering/unnamed_shadows_0x4a37d0.h - the obstacle debug drawing and the projected blob
// shadows (code 0x4a37d0-0x4a4ed7; source game/unnamed_shadows_0x4a37d0.cpp). The file name is a
// placeholder (no path string, no strings.tbl block); the directory is a guess (the shadows are drawn
// by levelrender_render_scene). Not original source; type and field names are ours unless
// sum.symbols names them. obstacles_debug_render, shadow_list_clear, frame_sleep_when_inactive and
// level_page_in_characters stay declared in engine/ai/boss_ai.h (decomp42).
#ifndef ENGINE_RENDERING_UNNAMED_SHADOWS_0X4A37D0_H
#define ENGINE_RENDERING_UNNAMED_SHADOWS_0X4A37D0_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/math/ix.h"
#include "vsdk/os/console.h"
#include "vsdk/gr/gr.h"
#include "engine/rendering/unnamed_projectors_0x4c0250.h"

struct lobj;
struct s3d_face;

// The projector and shadow records are in engine/rendering/unnamed_projectors_0x4c0250.h (decomp56; decomp57
// added shadow_record and the lobj_count bits); polygon_touches_planes / polygon_clip_to_planes in
// vsdk/math/ix.h (decomp56).

// A projected blob shadow (0x1f8, 16 at 0x245ce18; sum.symbols counts the records from +0x74, the
// shadow_record). Names ours.
struct blob_shadow {
    vector3 pos;                      // +0x00 projector origin
    matrix orient;                    // +0x0c (the projection runs along uvec)
    float tan_fov;                    // +0x30 tan of the half angle
    float near_dist;                  // +0x34
    float far_dist;                   // +0x38
    int bitmap;                       // +0x3c
    uint full_alpha : 1;              // +0x40 bit 0 particle mode, alpha 255 (else fades with distance)
    uint active : 1;                  //       bit 1
    uint _flag_rest : 30;
    vector3 local_pos;                // +0x44 in the current level object's frame
    matrix local_orient;              // +0x50
    shadow_record proj;               // +0x74
};
LAYOUT_CHECK_32(blob_shadow, sizeof(blob_shadow) == 0x1f8);
#define MAX_BLOB_SHADOWS 16

// ---- the unit -------------------------------------------------------------------------------------------
extern int obstacles_debug_size;                  // 0x59cd90 1 size class drawn (name ours)
extern int obstacles_debug_follow;                // 0x59cd94 1 highlight cur_character's layer (name ours)
extern int obstacles_debug_highlight;             // 0x245cde8
extern int obstacles_debug_all_layers;            // 0x245cdf8 also draw the other layers (name ours)
extern float obstacle_height_offset;              // 0x245ce04 (console obstacle_height_offset)
extern int obstacles_debug_axis_lines;            // 0x245ce08 draw the corner offsets through avoid_record_axis_at
extern console_command ccmd_obstacle_height_offset;   // 0x245cd90
extern blob_shadow blob_shadows[MAX_BLOB_SHADOWS];    // 0x245ce18
extern int num_blob_shadows;                      // 0x245f398

void cmd_obstacle_height_offset();                                            // 0x4a3e10
void shadow_record_add(const vector3 *pos, const matrix *orient, float fov, float near_dist, float far_dist,
                       int bitmap, const shadow_frame *frame, bool full_alpha);   // 0x4a3ef0
void shadows_render();                                                        // 0x4a4360
void shadow_render_on_face(blob_shadow *s, lobj *l, s3d_face *face);         // 0x4a44b0
gr_vertex *shadow_project_face(lobj *l, s3d_face *face, blob_shadow *s, const vector3 *points, int *count);   // 0x4a46a0
void shadow_transform_to_lobj(blob_shadow *s, lobj *l);                     // 0x4a4890
int  shadow_collect_for_lobj(lobj *l, shadow_record **list, int *count);    // 0x4a4b40 (list holds 64)

#endif
