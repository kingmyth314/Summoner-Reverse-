// unnamed_shadows_0x4a37d0.cpp - Sum.exe v1.40, code 0x4a37d0-0x4a4ed7: the obstacle debug drawing
// (render_3d_obstacles, the obstacle_height_offset command), the projected blob shadows (16 records
// projected onto the faces of the level objects they touch) and the draw-list ray test.
// FILE NAME NOT KNOWN: a placeholder (no path string, no strings.tbl block; docs/sum-exe/source-map.md
// hand cut 0x4a37d0-0x4a4ec0). Written from the Sum.exe disassembly (tools/asm.mjs; floats, strings and
// the EH FuncInfo read from Sum.exe). Target: VC++ 6.0 (cl 12.00.8966), optimised but WITHOUT inline
// expansion: vector3's inline constructor (0x43b580) and operator= (0x413190), matrix::operator=
// (0x4a4320) and plane::set_normal_point (0x43b5a0) are called, so their uses are written as the
// operators here (D500 style).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_exp_const_245cdc8 (0x4a37d0, initialiser table entry 292) to
// vector2::to_vector3 (0x4a4ec0-0x4a4ed7); the geometry unit starts with vector2::from_xz (0x4a4ee0).
// The handle-slot unit (game/unnamed_handleslots_0x4a3640.cpp) ends at 0x4a37c9.
// Data: .data 0x59cd8c-0x59ce85 (three debug ints, five floats, then the command's strings); .bss
// 0x245cd8c-0x245f39b. pf_debug_mode (0x59cd8c) and pf_debug_path (0x245cd8c), which pathfinding.cpp
// uses, are placed here by address: both are the first item of this unit's blocks (D3711, likely).
// shadow_render_on_face carries a profile_label (EH frame, FuncInfo 0x584148, only unwind action the
// empty destructor 0x434650).

#include <stdio.h>
#include <math.h>
#include "engine/rendering/unnamed_shadows_0x4a37d0.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/ai/pathfinding.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/objects/living_entity.h"
#include "summoner/player/player.h"
#include "levelscripts/savegame.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/profile.h"
#include "vsdk/gr/gr.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"

// ---- .data (0x59cd8c-) and .bss without initialisers (address order) ----------------------------------
int pf_debug_mode = 1;                        // 0x59cd8c compared with 7 by pf_graph_search
int obstacles_debug_size = 1;                 // 0x59cd90
int obstacles_debug_follow = 1;               // 0x59cd94
float shadow_float_59cd98 = 2.8f;             // 0x59cd98 (names ours: only the initialiser below reads one)
float shadow_float_59cd9c = 2.8f;             // 0x59cd9c
float shadow_float_59cda0 = 320.0f;           // 0x59cda0
float shadow_float_59cda4 = 240.0f;           // 0x59cda4
float shadow_float_59cda8 = 20.0f;            // 0x59cda8
ai_path *pf_debug_path;                       // 0x245cd8c freed by pathfinding_atexit_free
int obstacles_debug_highlight;                // 0x245cde8
int obstacles_debug_all_layers;               // 0x245cdf8
float obstacle_height_offset;                 // 0x245ce04
int obstacles_debug_axis_lines;               // 0x245ce08
int num_blob_shadows;                         // 0x245f398

// ---- objects with static initialisers (entries 292-297), in initialiser order -------------------------
// 0x4a37d0 static_init_exp_const_245cdc8 (jmp). Confidence: exact.
// 0x4a37e0 init_exp_const_245cdc8: exp() expanded with fldl2e / f2xm1 / fscale. Confidence: likely.
float shadow_exp_245cdc8 = (float)exp(shadow_float_59cd98);   // 0x245cdc8 (name ours, not read)
// 0x4a3810 static_init_copy_const_245cdcc (jmp). Confidence: exact.
// 0x4a3820 init_copy_const_245cdcc. Confidence: likely.
float shadow_exp_245cdcc = shadow_exp_245cdc8;                // 0x245cdcc (name ours, not read)
// 0x4a3830 static_init_empty_4a3840 (jmp, entry 294). Confidence: exact.
// 0x4a3840 init_empty_4a3840 (bare ret). Confidence: unsure (which object).
// 0x4a3850 static_init_empty_4a3860 (jmp, entry 295). Confidence: exact.
// 0x4a3860 init_empty_4a3860 (bare ret). Confidence: unsure (which object).
// Two objects whose constructor is empty; which (and their addresses) is a guess: two unreferenced
// vector3 in the gap 0x245cdd0-0x245cde7.
vector3 shadow_unref_vec_a;                   // 0x245cdd0 ? (name ours)
vector3 shadow_unref_vec_b;                   // 0x245cddc ? (name ours)

// 0x4a3870  Confidence: exact. state_game_frame passes (active, 1); the second argument is not read.
void frame_sleep_when_inactive(int active, int a)
{
    if (!active)
        timer_wait(10);
}

// Draws one nav segment at the debug height (the shared tail of obstacles_debug_render's loops).
#define OBSTACLE_LINE(pa, pb)                                                                           \
    do {                                                                                                \
        vector3 p0 = (pa)->to_vector3();                                                                \
        vector3 p1 = (pb)->to_vector3();                                                                \
        p0.y = p1.y = obstacle_height_offset + cur_character->pos.y + 0.01f;                             \
        gr_line_3d_world(&p0, &p1, gr_line_3d_mode);                                                    \
    } while (0)

// 0x4a3890  Confidence: likely (the point temporaries; the midpoint of the normal tick keeps a stale y
// in the code, written as the debug height here). game_render_frame when Render_3d_obstacles is set:
// the cell walls of the debug size class (the highlighted layer, or all with obstacles_debug_all_layers),
// the layer-transition edges, then for every used segment list of the highlighted layer its segments,
// their normals and the corner offsets. With obstacles_debug_axis_lines the second corner's offset is
// drawn from the first corner (the code adds corner a's position to both).
void obstacles_debug_render()
{
    pf_cell *cell;
    pf_edge *e;
    pf_wall *w;
    avoid_seglist *sl;
    avoid_seg_ref *s;
    avoid_corner *ca, *cb;
    vector2 *off;
    vector3 p0, p1, mid, tick;
    float y, mx, mz, nx, nz;
    int layer, i, k;

    if (obstacles_debug_follow)
        obstacles_debug_highlight = cur_character->layer;
    for (layer = 0; layer < PF_MAX_LAYERS; layer++) {
        if (layer == obstacles_debug_highlight)
            gr_set_color(0, 0xff, 0x40, 0xff);
        else if (obstacles_debug_all_layers)
            gr_set_color(0, 0x80, 0x20, 0xff);
        else
            continue;
        for (cell = pf_graphs[obstacles_debug_size][layer].cells; cell; cell = cell->next) {
            e = cell->edges;
            for (i = 0; i < cell->num_edges; i++, e++)
                OBSTACLE_LINE(e->a, e->b);
        }
    }
    gr_set_color(0x80, 0x20, 0, 0xff);
    w = pf_walls;
    for (i = 0; i < pf_wall_count; i++, w++)
        OBSTACLE_LINE(w->a, w->b);
    for (sl = avoid_seglists; sl < &avoid_seglists[AVOID_NUM_SEGLISTS]; sl++) {
        if (!(sl->flags & AVOID_SEGLIST_USED))
            continue;
        if (sl->flags & AVOID_SEGLIST_DOOR)
            gr_set_color(0x20, 0x20, 0x80, 0xff);
        else
            gr_set_color(0x80, 0x20, 0x20, 0xff);
        if (sl->layer != obstacles_debug_highlight || sl->size != obstacles_debug_size)
            continue;
        for (k = 0; k < sl->num_segs; k++) {
            s = &sl->segs[k];
            ca = &sl->corners[s->a];
            cb = &sl->corners[s->b];
            y = obstacle_height_offset + cur_character->pos.y + 0.01f;
            p0.x = ca->pos.x;
            p0.y = y;
            p0.z = ca->pos.y;
            p1.x = cb->pos.x;
            p1.y = y;
            p1.z = cb->pos.y;
            gr_line_3d_world(&p0, &p1, gr_line_3d_mode);
            mx = (cb->pos.x + ca->pos.x) * 0.5f;
            mz = (cb->pos.y + ca->pos.y) * 0.5f;
            nx = s->normal.x * 0.5f;
            nz = s->normal.y * 0.5f;
            mid.x = mx;
            mid.y = y;
            mid.z = mz;
            tick.x = mx + nx;
            tick.y = y;
            tick.z = mz + nz;
            gr_line_3d_world(&mid, &tick, gr_line_3d_mode);
            if (obstacles_debug_axis_lines) {
                off = avoid_record_axis_at(sl, s->a);
                p0.x = off->x * 0.5f + ca->pos.x;
                p0.z = off->y * 0.5f + ca->pos.y;
                off = avoid_record_axis_at(sl, s->b);
                p1.x = off->x * 0.5f + ca->pos.x;
                p1.z = off->y * 0.5f + ca->pos.y;
            } else {
                p0.x = ca->offset.x * 0.5f + ca->pos.x;
                p0.z = ca->offset.y * 0.5f + ca->pos.y;
                p1.x = cb->offset.x * 0.5f + cb->pos.x;
                p1.z = cb->offset.y * 0.5f + cb->pos.y;
            }
            mid.x = ca->pos.x;
            mid.z = ca->pos.y;
            gr_line_3d_world(&mid, &p0, gr_line_3d_mode);
            mid.x = cb->pos.x;
            mid.z = cb->pos.y;
            gr_line_3d_world(&mid, &p1, gr_line_3d_mode);
        }
    }
}

// 0x4a3de0 static_init_ccmd_obstacle_height_offset (jmp, initialiser entry 296). Confidence: exact.
// 0x4a3df0 init_ccmd_obstacle_height_offset (console_command constructor on 0x245cd90). Confidence: exact.
console_command ccmd_obstacle_height_offset("obstacle_height_offset",
                                            "Set the height level to draw obstacles relative to main player pos",
                                            cmd_obstacle_height_offset);   // 0x245cd90

// 0x4a3e10  Confidence: exact.
void cmd_obstacle_height_offset()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        obstacle_height_offset = command_argument_float;
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "Obstacle height offset currently at %.3f meters.\n", obstacle_height_offset);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: obstacle_height_offset <distance> -- where distance is in meters.\n", 0);
}

// 0x4a3e80 static_init_shadow_records (jmp, initialiser entry 297). Confidence: exact.
// 0x4a3e90 init_shadow_records  Confidence: exact. The implicit constructor of each record, i.e.
// shadow_record's (0x4c0690) on +0x74, inline in a 16-step loop.
blob_shadow blob_shadows[MAX_BLOB_SHADOWS];   // 0x245ce18

// 0x4a3eb0  Confidence: exact. state_game_frame, before the characters add theirs.
void shadow_list_clear()
{
    int i;

    for (i = 0; i < num_blob_shadows; i++)
        blob_shadows[i].proj.lobj_count = 0;
    num_blob_shadows = 0;
}

// 0x4a3ef0  Confidence: likely (the six frustum normals are built in the order the code stores them).
// shadow_draw_character: the next record (16 at most) with a frustum of four side planes through pos
// (half angle fov), a near and a far plane, and a sphere at the middle of the depth range.
void shadow_record_add(const vector3 *pos, const matrix *orient, float fov, float near_dist, float far_dist,
                       int bitmap, const shadow_frame *frame, bool full_alpha)
{
    blob_shadow *s;
    vector3 n_left, n_right, n_front, n_back, n_down, n_up, centre;
    plane pl;
    float h;

    if (num_blob_shadows >= MAX_BLOB_SHADOWS)
        return;
    s = &blob_shadows[num_blob_shadows++];
    s->pos.x = pos->x;
    s->pos.y = pos->y;
    s->pos.z = pos->z;
    s->orient.rvec = orient->rvec;
    s->orient.uvec = orient->uvec;
    s->orient.fvec = orient->fvec;
    s->tan_fov = (float)tan(fov);
    s->near_dist = near_dist;
    s->far_dist = far_dist;
    s->bitmap = bitmap;
    s->full_alpha = full_alpha;
    s->active = 1;
    if (frame)
        s->proj.frame = *frame;
    n_left.x = -1.0f;
    n_left.y = -s->tan_fov;
    n_left.z = 0.0f;
    n_right.x = 1.0f;
    n_right.y = -s->tan_fov;
    n_right.z = 0.0f;
    n_front.x = 0.0f;
    n_front.y = -s->tan_fov;
    n_front.z = -1.0f;
    n_back.x = 0.0f;
    n_back.y = -s->tan_fov;
    n_back.z = 1.0f;
    n_down.x = 0.0f;
    n_down.y = -1.0f;
    n_down.z = 0.0f;
    n_up.x = 0.0f;
    n_up.y = 1.0f;
    n_up.z = 0.0f;
    n_left.unrotate(s->orient);
    n_right.unrotate(s->orient);
    n_front.unrotate(s->orient);
    n_back.unrotate(s->orient);
    n_down.unrotate(s->orient);
    n_up.unrotate(s->orient);
    n_left.normalize();
    n_right.normalize();
    n_front.normalize();
    n_back.normalize();
    n_down.normalize();
    n_up.normalize();
    h = (near_dist + far_dist) * 0.5f;
    centre.x = h * s->orient.uvec.x + pos->x;
    centre.y = h * s->orient.uvec.y + pos->y;
    centre.z = h * s->orient.uvec.z + pos->z;
    s->proj.clear_planes();
    pl.set_normal_point(&n_left, pos);
    s->proj.set(&centre, (far_dist - near_dist) * 0.5f, pl);
    pl.set_normal_point(&n_right, pos);
    s->proj.add_plane(pl);
    pl.set_normal_point(&n_front, pos);
    s->proj.add_plane(pl);
    pl.set_normal_point(&n_back, pos);
    s->proj.add_plane(pl);
    vector3 near_pt(n_up.x * s->near_dist + pos->x, n_up.y * s->near_dist + pos->y, n_up.z * s->near_dist + pos->z);
    pl.set_normal_point(&n_down, &near_pt);
    s->proj.add_plane(pl);
    vector3 far_pt(n_up.x * s->far_dist + pos->x, n_up.y * s->far_dist + pos->y, n_up.z * s->far_dist + pos->z);
    s->proj.add_plane(plane(&n_up, &far_pt));
}

// 0x4a42e0 plane::plane(normal, point) (sum.symbols plane__plane, D3601). Confidence: exact.
// 0x4a4320 matrix::operator= (matrix3__copy). Confidence: exact.
// Both are COMDATs of inline members (vsdk/math/ix.h, vsdk/math/matrix.h) emitted with this unit.

// 0x4a4360  Confidence: likely. levelrender_render_scene: every record over the faces of the level
// objects it was projected into that are marked in the face bitmap.
void shadows_render()
{
    blob_shadow *s;
    lobj *l;
    s3d_geometry *g;
    ubyte *marks;
    uint m;
    int i, k, index, nbytes, b, bit;

    if (!shadows_enabled || num_blob_shadows <= 0)
        return;
    s = blob_shadows;
    for (i = num_blob_shadows; i; i--, s++) {
        for (k = 0; k < (int)s->proj.lobj_count; k++) {
            l = s->proj.lobj_list[k];
            index = l - lobjs;
            shadow_transform_to_lobj(s, l);
            g = l->mesh;
            marks = &lobj_face_marks[lobj_face_mark_offsets[index]];
            nbytes = (g->num_faces + 7) / 8;
            for (b = 0; b < nbytes; b++) {
                m = *marks++;
                if (!m)
                    continue;
                for (bit = 0; bit < 8; bit++) {
                    if (m & 1)
                        shadow_render_on_face(s, l, &g->faces[b * 8 + bit]);
                    m >>= 1;
                }
            }
        }
    }
}

// 0x4a44b0  Confidence: likely (the profile label). An active record on one face: the face clipped to
// the frustum and drawn with the shadow bitmap (shadow mode, or particle alpha with full_alpha).
void shadow_render_on_face(blob_shadow *s, lobj *l, s3d_face *face)
{
    profile_label label("shadow_render_on_face");
    vector3 pts[3];
    gr_vertex *vp[32];
    gr_vertex *v;
    int n, i;

    if (!s->active)
        return;
    pts[0] = l->mesh->vertices[face->v[0]];
    pts[1] = l->mesh->vertices[face->v[1]];
    pts[2] = l->mesh->vertices[face->v[2]];
    if (!polygon_touches_planes(3, pts, s->proj.local_planes, s->proj.num_planes, 0))
        return;
    v = shadow_project_face(l, face, s, pts, &n);
    if (!v || n < 3)
        return;
    for (i = 0; i < n; i++)
        vp[i] = &v[i];
    gr_poly_begin(0, 0, 0, 0, 0);
    for (i = 0; i < n; i++)
        gr_poly_vertex(vp[i]);
    if (s->full_alpha)
        gr_poly_set_texture(gr_mode_particle_alpha, s->bitmap, 0xff, 0xff, 0xff, 0xff, 0);
    else
        gr_poly_set_texture(gr_mode_shadow, s->bitmap, 0, 0, 0, 0xff, 0);
    for (i = 0; i < n; i++) {
        gr_poly_uv(vp[i]->u, vp[i]->v);
        gr_poly_alpha(vp[i]->a);
    }
    gr_poly_end();
}

// The vertex buffer of shadow_project_face: a function-local static with a guard bit (0x245ce10) and an
// atexit destructor that is a bare ret (0x4a4880 atexit_nop_4a4880), i.e. a type with an empty
// destructor; the wrapper is ours (unsure).
struct shadow_vertex_buffer {
    gr_vertex v[32];
    ~shadow_vertex_buffer() {}
};

// 0x4a46a0  Confidence: likely (the vertex buffer type). Clips the face to the frustum, then per vertex
// the view-space vertex (pushed 0.07 towards the viewer), the projected uv and the alpha (255 in
// full_alpha mode, else fading over the depth range). Returns the buffer and *count, NULL when clipped
// away.
gr_vertex *shadow_project_face(lobj *l, s3d_face *face, blob_shadow *s, const vector3 *points, int *count)
{
    static shadow_vertex_buffer buf;          // 0x245ed98 (guard byte 0x245ce10)
    vector3 clipped[32];
    vector3 d, pr, world, w;
    gr_vertex *v;
    float f;
    int i;

    *count = 3;
    *count = polygon_clip_to_planes(3, points, clipped, s->proj.local_planes, s->proj.num_planes);
    if (!*count)
        return NULL;
    v = buf.v;
    for (i = 0; i < *count; i++, v++) {
        d.x = clipped[i].x - s->local_pos.x;
        d.y = clipped[i].y - s->local_pos.y;
        d.z = clipped[i].z - s->local_pos.z;
        pr = d.rotated(s->local_orient);
        w = clipped[i].unrotated(l->orient);
        world.x = w.x + l->pos.x;
        world.y = w.y + l->pos.y;
        world.z = w.z + l->pos.z;
        gr_rotate_vertex(v, &world);
        gr_offset_point_depth(v, -0.07f);
        v->u = (pr.x / (pr.y * s->tan_fov) + 1.0f) * 0.5f;
        v->v = 0.5f - pr.z / (pr.y * s->tan_fov) * 0.5f;
        if (s->full_alpha) {
            v->a = 0xff;
        } else {
            f = 1.5f - (pr.y - s->near_dist) * 1.5f / (s->far_dist - s->near_dist);
            if (f > 0.0f) {
                if (f >= 1.0f)
                    f = 1.0f;
            } else {
                f = 0.0f;
            }
            v->a = (ubyte)(sqrt(f) * 255.0f);
        }
    }
    (void)face;
    return buf.v;
}

// 0x4a4880 atexit_nop_4a4880  Confidence: likely. The static's atexit destructor
// (shadow_vertex_buffer's empty destructor, expanded).

// 0x4a4890  Confidence: likely (the discarded vector3 copies before each rotate are the code's). The
// record's frame relative to the level object: doors use their current pose (the move timer eased in
// and out between the closed and the open pose, or the resting pose).
void shadow_transform_to_lobj(blob_shadow *s, lobj *l)
{
    door_info *door;
    matrix m;
    quaternion q;
    vector3 pos;
    bool open, locked;
    float f, g;

    s->proj.to_lobj_space(l);
    if (l->flags & 0x100) {
        door = door_from_lobj(l);
        if (!door)
            return;
        savegame_state.two_stage_get(&open, &locked, l);
        if (door->state & 1) {
            f = door->timer.GetElapsedTime() * 0.001f / door->_unk08;
            if (open)
                f = 1.0f - f;
            if (f > 0.5f)
                f = f * 4.0f - f * f * 2.0f - 1.0f;
            else
                f = f * f * 2.0f;
            g = 1.0f - f;
            pos.x = g * door->closed_pos.x + f * door->open_pos.x;
            pos.y = g * door->closed_pos.y + f * door->open_pos.y;
            pos.z = g * door->closed_pos.z + f * door->open_pos.z;
            quaternion_slerp(&q, &door->open_orient, &door->closed_orient, g);
            q.to_matrix_b(&m);
        } else if (open) {
            pos.x = door->open_pos.x;
            pos.y = door->open_pos.y;
            pos.z = door->open_pos.z;
            door->open_orient.to_matrix_b(&m);
        } else {
            pos.x = door->closed_pos.x;
            pos.y = door->closed_pos.y;
            pos.z = door->closed_pos.z;
            door->closed_orient.to_matrix_b(&m);
        }
        s->local_pos.x = s->pos.x - pos.x;
        s->local_pos.y = s->pos.y - pos.y;
        s->local_pos.z = s->pos.z - pos.z;
        vector3 unused(s->local_pos);
        s->local_pos.rotate(m);
        s->local_orient.rvec = s->orient.rvec;
        s->local_orient.uvec = s->orient.uvec;
        s->local_orient.fvec = s->orient.fvec;
        s->local_orient.fvec.unrotate(m);
        s->local_orient.uvec.unrotate(m);
        s->local_orient.rvec.unrotate(m);
        return;
    }
    vector3 d(s->pos.x - l->pos.x, s->pos.y - l->pos.y, s->pos.z - l->pos.z);
    s->local_pos.x = d.x;
    s->local_pos.y = d.y;
    s->local_pos.z = d.z;
    vector3 unused2(s->local_pos);
    s->local_pos.rotate(l->orient);
    s->local_orient = s->orient;
    s->local_orient.fvec.rotate(l->orient);
    s->local_orient.uvec.rotate(l->orient);
    s->local_orient.rvec.rotate(l->orient);
}

// 0x4a4b40  Confidence: exact. levelrender_lobj_collect_projections: the records whose frustum holds
// the object's sphere (at most 64 listed); returns how many matched.
int shadow_collect_for_lobj(lobj *l, shadow_record **list, int *count)
{
    blob_shadow *s;
    int n, i;

    n = 0;
    s = blob_shadows;
    for (i = num_blob_shadows; i > 0; i--, s++) {
        s->proj.test_bounds(l);
        if (s->proj.inside) {
            if ((uint)*count < 0x40) {
                list[*count] = &s->proj;
                (*count)++;
            }
            n++;
        }
    }
    return n;
}

// 0x4a4bb0  Confidence: likely (the stage byte goes through a char pointer). Ray against the level
// objects: with no attachment entries every object from *index on (unless attach_only); else the
// attachment, opaque and sorted draw lists in turn (*stage 0, 1, 2), objects within the fog distance and
// at least ray->min_radius big. On a hit *index is the next position to resume from; returns the hit
// count.
int lobj_raycast_draw_lists(lobj_ray *ray, lobj_hit *hit, int *index, bool *blocked, bool attach_only)
{
    levelrender_entry *e;
    char *stage;
    char local_stage;
    float fog_far;
    bool first;
    int local_index, n, i;

    fog_far = fog_get_far_current();
    if (!index) {
        local_index = 0;
        index = &local_index;
    }
    stage = (char *)blocked;
    if (!stage) {
        local_stage = 0;
        stage = &local_stage;
    }
    first = true;
    hit->num_hits = 0;
    hit->_unk20 = 0;
    hit->_unk28 = 0;
    if (levelrender_attach_count <= 0) {
        if (attach_only)
            return 0;
        n = lobj_get_count();
        hit->num_hits = 0;
        for (i = *index; i < n; i++) {
            if (lobj_from_index(i)->mesh->_unk30 < ray->min_radius)
                continue;
            lobj_raycast(i, ray, hit, first);
            first = false;
            if (hit->num_hits > 0) {
                *index = i + 1;
                return hit->num_hits;
            }
        }
        return hit->num_hits;
    }
    first = true;
    if (*stage == 0) {
        for (i = *index; i < levelrender_attach_count; i++) {
            e = levelrender_attach_list[i];
            if (e->type != 1 || e->dist > fog_far)
                continue;
            if (lobj_from_index(e->lobj)->mesh->_unk30 < ray->min_radius)
                continue;
            lobj_raycast(e->lobj, ray, hit, first);
            first = false;
            if (hit->num_hits > 0) {
                *index = i + 1;
                return hit->num_hits;
            }
        }
        *stage = 1;
        *index = 0;
    }
    if (*stage == 1) {
        for (i = *index; i < levelrender_opaque_count; i++) {
            e = levelrender_opaque_list[i];
            if (e->type != 1 || e->dist > fog_far)
                continue;
            if (lobj_from_index(e->lobj)->mesh->_unk30 < ray->min_radius)
                continue;
            lobj_raycast(e->lobj, ray, hit, first);
            first = false;
            if (hit->num_hits > 0) {
                *index = i + 1;
                return hit->num_hits;
            }
        }
        *stage = 2;
        *index = 0;
    }
    for (i = *index; i < levelrender_sort_count; i++) {
        e = levelrender_sort_list[i];
        if (e->type != 1 || e->dist > fog_far)
            continue;
        if (lobj_from_index(e->lobj)->mesh->_unk30 < ray->min_radius)
            continue;
        lobj_raycast(e->lobj, ray, hit, first);
        first = false;
        if (hit->num_hits > 0) {
            *index = i + 1;
            return hit->num_hits;
        }
    }
    return 0;
}

// 0x4a4e60  Confidence: exact. Static member, cdecl: both components within 0.001.
bool vector2::nearly_equal(const vector2 *a, const vector2 *b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    if (dx < 0.001f && dx > -0.001f && dy < 0.001f && dy > -0.001f)
        return true;
    return false;
}

// 0x4a4ec0  Confidence: exact. COMDAT of the member (thiscall, hidden result).
vector3 vector2::to_vector3() const
{
    vector3 r;
    r.x = x;
    r.y = 0.0f;
    r.z = y;
    return r;
}
