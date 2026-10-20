// unnamed_gr_line_0x53e520.cpp - Sum.exe v1.40, code 0x53e520-0x54080f: the 3D drawing helpers of the
// renderer front end (3D lines and their back-end dispatch, gr_poly with the buffered alpha polygons and
// its console command, camera-facing and oriented bitmaps, line quads, the debug sphere, box and arrows;
// vsdk\gr in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp69 NOTES.md, D5001): cmdline.cpp (vsdk\os) ends with cmdline_get_unclaimed_arg
// (0x53e4e0); this object runs from gr_line_3d to the gr::Vertex assignment COMDAT (0x540790), and
// character.cpp starts with its initialiser at 0x540810. Its one static initialiser (the console command,
// table 0x5889ec) lies after the first two functions, where the command is defined. Data: the command's
// literals 0x5abfe0-0x5ac0dd and .bss 0x2dc7060-0x2dc706f (after cmdline.cpp's 0x2dc7054).
//
// Like the other vsdk objects this one calls the header inlines out of line (vector3 and matrix
// operators, gr::Mode, gr_clip_free_temp_verts), so it was built without inline expansion (/Ob0 in
// vc6-flags.txt, D7101). gr_vertex's position is a vector3 base (D7100): the binary runs gr__Vertex__Vertex
// (0x5081c0) on every local vertex and the vertex assignments of gr_draw_sphere call the implicit
// operator= COMDAT 0x540790 (which assigns the vector3 at +0 through vector3::operator=).
//
// Bug (NOTES.md): gr_bitmap_3d_projected and gr_bitmap_3d_oriented_quad test `!pnt->flags & 1` (the
// precedence slip of !(pnt->flags & 1)), which never fails after gr_project_vertex.

#include <math.h>
#include <stdio.h>
#include "vsdk/os/console.h"
#include "vsdk/os/mouse.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/unnamed_clip_0x5590b0.h"
#include "vsdk/gr/unnamed_gr_poly_0x5450c0.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "vsdk/gr/glide/gr_glide.h"
#include "vsdk/gr/opengl/gr_opengl.h"
#include "vsdk/gr/unnamed_gr_line_0x53e520.h"

// 2 pi as the float constant 0x580e28.
#define GR_LINE_2PI 6.2831855f

// 0x53e520  Confidence: likely (the result is kept in a variable; the clipped path frees the temporary
// vertices). False when both points are outside one clip plane, or still outside after clipping.
bool gr_line_3d(gr_vertex *p0, gr_vertex *p1, gr::Mode mode)
{
    ubyte codes;
    bool clipped = false;
    bool ret = true;

    if (p0->codes & p1->codes)
        return false;
    codes = p0->codes | p1->codes;
    if (codes) {
        gr_clip_line(&p0, &p1, codes);
        clipped = true;
        if (p0->codes | p1->codes) {
            ret = false;
            gr_clip_free_temp_verts();
            return ret;
        }
    }
    if (!(p0->flags & 1))
        gr_project_vertex(p0);
    if (!(p1->flags & 1))
        gr_project_vertex(p1);
    switch (gr_screen.mode) {
    case GR_DIRECT3D:
        gr_d3d_line_3d(p0, p1, mode);
        break;
    case GR_GLIDE:
        gr_glide_line_3d(p0, p1, mode);
        break;
    case GR_OPENGL:
        gr_opengl_line_3d(p0, p1, mode);
        break;
    }
    if (clipped)
        gr_clip_free_temp_verts();
    return ret;
}

// 0x53e610  Confidence: exact (the two vertices are constructed in the binary, D1605).
void gr_line_3d_world(const vector3 *a, const vector3 *b, gr::Mode mode)
{
    gr_vertex p0, p1;

    p0._unk1a = 0;
    p1._unk1a = 0;
    gr_rotate_vertex(&p0, a);
    gr_rotate_vertex(&p1, b);
    gr_line_3d(&p0, &p1, mode);
}

// ---- console command "use_buffered_alpha_polys" ----------------------------------------------------------
int Use_buffered_alpha_polys;                           // 0x2dc706c
// 0x53e670 init_ccmd_use_buffered_alpha_polys_thunk (jmp, initialiser table 0x5889ec): compiler-generated.
// Confidence: likely.
// 0x53e680 init_ccmd_use_buffered_alpha_polys: the dynamic initialiser of this definition. Confidence: exact.
console_command ccmd_use_buffered_alpha_polys("use_buffered_alpha_polys", "Toggles Use_buffered_alpha_polys",
                                              cmd_use_buffered_alpha_polys);   // 0x2dc7060

// 0x53e6a0  Confidence: exact. The console's bool command macro (as cmd_load_mipmaps in bmpman.cpp).
void cmd_use_buffered_alpha_polys()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Use_buffered_alpha_polys = 1;
        else if (arg_type & ARG_FALSE)
            Use_buffered_alpha_polys = 0;
        else if (arg_type & ARG_NONE)
            Use_buffered_alpha_polys ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "use_buffered_alpha_polys", "Use_buffered_alpha_polys");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "use_buffered_alpha_polys", Use_buffered_alpha_polys ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x53e760  Confidence: exact. LoadSettings.
void gr_set_use_buffered_alpha_polys(bool on)
{
    Use_buffered_alpha_polys = on;
}

// 0x53e770  Confidence: likely (the two unbuffered begin calls share their tail in the binary). With the
// buffered alpha polygons on, a polygon with a translucent vertex alpha (or colour alpha) or an alpha
// source 1 / 3 is recorded with its largest view z as the sort key.
bool gr_poly(int n, gr_vertex **verts, gr::Mode mode, bool fixed_depth, float depth)
{
    int alpha = mode.get_alpha_source();
    bool translucent = false;
    float z;
    int i;

    if (Use_buffered_alpha_polys) {
        for (i = 0; !translucent && i < n; i++) {
            if (verts[i]->_unk1a & GR_VERTEX_ALPHA) {
                if (verts[i]->a != 0xff)
                    translucent = true;
            } else if (gr_screen.current_color.a != 0xff) {
                translucent = true;
            }
        }
        if (translucent || alpha == 3 || alpha == 1) {
            z = verts[0]->z;
            for (i = 1; i < n; i++) {
                if (verts[i]->z > z)
                    z = verts[i]->z;
            }
            gr_poly_begin(true, z, gr_screen.poly_arg_a4, fixed_depth, depth);
        } else {
            gr_poly_begin(false, 0.0f, gr_screen.poly_arg_a4, fixed_depth, depth);
        }
    } else {
        gr_poly_begin(false, 0.0f, gr_screen.poly_arg_a4, fixed_depth, depth);
    }
    gr_poly_set_texture(mode, gr_screen.current_bitmap, gr_screen.current_color.r, gr_screen.current_color.g,
                        gr_screen.current_color.b, gr_screen.current_color.a, 0);
    for (i = 0; i < n; i++) {
        gr_poly_vertex(verts[i]);
        if (verts[i]->_unk1a & GR_VERTEX_UV)
            gr_poly_uv(verts[i]->u, verts[i]->v);
        if (verts[i]->_unk1a & GR_VERTEX_RGB)
            gr_poly_rgb(verts[i]->r, verts[i]->g, verts[i]->b);
        if (verts[i]->_unk1a & GR_VERTEX_ALPHA)
            gr_poly_alpha(verts[i]->a);
    }
    return gr_poly_end();
}

// 0x53e8d0  Confidence: exact (VC6 8966 byte match, w10: codes_or first, the vertex through a local). No caller. A polygon still outside one plane after clipping returns its
// vertex count without writing out (dead code).
int gr_poly_project_to_screen(int n, gr_vertex **verts, vector2 *out)
{
    gr_vertex **list = gr_clip_vlist_a;
    ubyte codes[2];
    ubyte codes_or = 0, codes_and = 0xff;
    bool clipped;
    int i;

    for (i = 0; i < n; i++) {
        list[i] = verts[i];
        codes_and &= list[i]->codes;
        codes_or |= list[i]->codes;
    }
    if (codes_and)
        return 0;
    codes[0] = codes_or;
    codes[1] = 0;
    clipped = false;
    if (codes_or) {
        clipped = true;
        list = gr_clip_polygon(gr_clip_vlist_a, gr_clip_vlist_b, &n, codes);
        if (n == 0 || codes[1])
            goto done;
    }
    for (i = 0; i < n; i++) {
        gr_vertex *v = list[i];
        if (!(v->flags & 1))
            gr_project_vertex(v);
        if (v->flags & 2)
            return 0;
        out[i].x = v->sx;
        out[i].y = v->sy;
    }
done:
    if (clipped)
        gr_clip_free_temp_verts();
    return n;
}

// 0x53e9d0  Confidence: exact (VC6 8966 byte match, w10: codes_or declared before codes_and). The result list replaces verts (the caller's array has room for the
// clipped vertices). A vertex behind the viewer returns 0 without freeing the temporary vertices.
int gr_clip_and_project_poly(int n, gr_vertex **verts)
{
    gr_vertex **list = gr_clip_vlist_a;
    ubyte codes[2];
    ubyte codes_or = 0, codes_and = 0xff;
    bool clipped;
    int i;

    for (i = 0; i < n; i++) {
        list[i] = verts[i];
        codes_and &= list[i]->codes;
        codes_or |= list[i]->codes;
    }
    if (codes_and)
        return 0;
    codes[0] = codes_or;
    codes[1] = 0;
    clipped = false;
    if (codes_or) {
        clipped = true;
        list = gr_clip_polygon(gr_clip_vlist_a, gr_clip_vlist_b, &n, codes);
        if (n == 0 || codes[1]) {
            n = 0;
            goto done;
        }
    }
    for (i = 0; i < n; i++) {
        gr_vertex *v = list[i];
        if (!(v->flags & 1))
            gr_project_vertex(v);
        if (v->flags & 2)
            return 0;
        verts[i] = list[i];
    }
done:
    if (clipped)
        gr_clip_free_temp_verts();
    return n;
}

// 0x53eae0  Confidence: likely (FreeSpace's g3_draw_rotated_bitmap in shape; the larger side of the bitmap
// is scaled up, the v coordinates are flipped and the list runs backwards). The depth is the centre's,
// pulled towards the viewer by the size scaled with gr_scale_by_far_clip.
bool gr_bitmap_3d_projected(gr_vertex *pnt, float angle, float size, gr::Mode mode)
{
    gr_vertex v[4];
    gr_vertex *list[4];
    float sa, ca, width, height, depth, off;
    int bw, bh, i;
    ubyte codes_and;

    list[0] = &v[3];
    list[1] = &v[2];
    list[2] = &v[1];
    list[3] = &v[0];
    if (!(pnt->flags & 1))
        gr_project_vertex(pnt);
    if (!pnt->flags & 1)
        return false;
    sa = (float)sin(angle);
    ca = (float)cos(angle);
    width = height = size;
    if (gr_screen.current_bitmap > -1) {
        bm_get_dimensions(gr_screen.current_bitmap, &bw, &bh);
        if (bw == 0)
            return false;
        if (bw < bh)
            height = size * bh / bw;
        else if (bw > bh)
            width = size * bw / bh;
    }
    v[0].x = (height * sa - width * ca) * gr_matrix_scale.x + pnt->x;
    v[0].y = (-width * sa - height * ca) * gr_matrix_scale.y + pnt->y;
    v[0].z = pnt->z;
    v[0].u = 0.0f;
    v[0].v = 1.0f;
    v[0]._unk1a = GR_VERTEX_UV;
    v[1].x = (width * ca + height * sa) * gr_matrix_scale.x + pnt->x;
    v[1].y = (width * sa - height * ca) * gr_matrix_scale.y + pnt->y;
    v[1].z = pnt->z;
    v[1].u = 1.0f;
    v[1].v = 1.0f;
    v[1]._unk1a = GR_VERTEX_UV;
    v[2].x = (width * ca - height * sa) * gr_matrix_scale.x + pnt->x;
    v[2].y = (height * ca + width * sa) * gr_matrix_scale.y + pnt->y;
    v[2].z = pnt->z;
    v[2].u = 1.0f;
    v[2].v = 0.0f;
    v[2]._unk1a = GR_VERTEX_UV;
    v[3].x = (-width * ca - height * sa) * gr_matrix_scale.x + pnt->x;
    v[3].y = (height * ca - width * sa) * gr_matrix_scale.y + pnt->y;
    v[3].z = pnt->z;
    v[3].u = 0.0f;
    v[3].v = 0.0f;
    v[3]._unk1a = GR_VERTEX_UV;
    depth = pnt->z;
    off = gr_scale_by_far_clip(size);
    if (off < depth)
        depth -= off;
    codes_and = 0xff;
    for (i = 0; i < 4; i++) {
        codes_and &= gr_code_vertex(&v[i]);
        v[i].flags = 0;
    }
    if (codes_and)
        return false;
    gr_noop_507d20(0x10);
    return gr_poly(4, list, mode, true, depth);
}

// 0x53edd0  Confidence: likely (as gr_bitmap_3d_projected with the given size, halved in place; no bitmap
// lookup).
bool gr_bitmap_3d_oriented_quad(gr_vertex *pnt, float angle, float width, float height, gr::Mode mode)
{
    gr_vertex v[4];
    gr_vertex *list[4];
    float sa, ca, depth, off;
    int i;
    ubyte codes_and;

    list[0] = &v[3];
    list[1] = &v[2];
    list[2] = &v[1];
    list[3] = &v[0];
    if (!(pnt->flags & 1))
        gr_project_vertex(pnt);
    if (!pnt->flags & 1)
        return false;
    sa = (float)sin(angle);
    ca = (float)cos(angle);
    width *= 0.5f;
    height *= 0.5f;
    v[0].x = (height * sa - width * ca) * gr_matrix_scale.x + pnt->x;
    v[0].y = (-width * sa - height * ca) * gr_matrix_scale.y + pnt->y;
    v[0].z = pnt->z;
    v[0].u = 0.0f;
    v[0].v = 1.0f;
    v[0]._unk1a = GR_VERTEX_UV;
    v[1].x = (width * ca + height * sa) * gr_matrix_scale.x + pnt->x;
    v[1].y = (width * sa - height * ca) * gr_matrix_scale.y + pnt->y;
    v[1].z = pnt->z;
    v[1].u = 1.0f;
    v[1].v = 1.0f;
    v[1]._unk1a = GR_VERTEX_UV;
    v[2].x = (width * ca - height * sa) * gr_matrix_scale.x + pnt->x;
    v[2].y = (height * ca + width * sa) * gr_matrix_scale.y + pnt->y;
    v[2].z = pnt->z;
    v[2].u = 1.0f;
    v[2].v = 0.0f;
    v[2]._unk1a = GR_VERTEX_UV;
    v[3].x = (-width * ca - height * sa) * gr_matrix_scale.x + pnt->x;
    v[3].y = (height * ca - width * sa) * gr_matrix_scale.y + pnt->y;
    v[3].z = pnt->z;
    v[3].u = 0.0f;
    v[3].v = 0.0f;
    v[3]._unk1a = GR_VERTEX_UV;
    depth = pnt->z;
    off = gr_scale_by_far_clip(width);
    if (off < depth)
        depth -= off;
    codes_and = 0xff;
    for (i = 0; i < 4; i++) {
        codes_and &= gr_code_vertex(&v[i]);
        v[i].flags = 0;
    }
    if (codes_and)
        return false;
    return gr_poly(4, list, mode, true, depth);
}

// 0x53f060  Confidence: likely (FreeSpace's g3_draw_polygon in shape: the corners pos +- rvec * w +- uvec
// * h, w / h the size scaled by the bitmap's proportions). No caller.
bool gr_bitmap_3d_oriented_quad_facing(const vector3 *pos, const matrix *orient, float size, gr::Mode mode)
{
    float w, h;
    int bw, bh;

    if (!gr_check_normal_facing(&orient->fvec, pos))
        return false;
    gr_vertex v[4];
    gr_vertex *list[4];
    list[0] = &v[3];
    list[1] = &v[2];
    list[2] = &v[1];
    list[3] = &v[0];
    if (gr_screen.current_bitmap > -1) {
        bm_get_dimensions(gr_screen.current_bitmap, &bw, &bh);
        if (bw == 0)
            return false;
        if (bw < bh) {
            w = size;
            h = size * bh / bw;
        } else if (bw > bh) {
            h = size;
            w = size * bw / bh;
        } else {
            h = size;
            w = size;
        }
    } else {
        h = size;
        w = size;
    }
    vector3 p;
    p = *pos + orient->rvec * -w + orient->uvec * h;
    gr_rotate_vertex(&v[0], &p);
    v[0].u = 1.0f;
    v[0].v = 0.0f;
    v[0]._unk1a = GR_VERTEX_UV;
    p = *pos + orient->rvec * w + orient->uvec * h;
    gr_rotate_vertex(&v[1], &p);
    v[1].u = 0.0f;
    v[1].v = 0.0f;
    v[1]._unk1a = GR_VERTEX_UV;
    p = *pos + orient->rvec * w + orient->uvec * -h;
    gr_rotate_vertex(&v[2], &p);
    v[2].u = 0.0f;
    v[2].v = 1.0f;
    v[2]._unk1a = GR_VERTEX_UV;
    p = *pos + orient->rvec * -w + orient->uvec * -h;
    gr_rotate_vertex(&v[3], &p);
    v[3].u = 1.0f;
    v[3].v = 1.0f;
    v[3]._unk1a = GR_VERTEX_UV;
    return gr_poly(4, list, mode, false, 0.0f);
}

// 0x53f3b0  Confidence: exact (the binary constructs the vertex, D1605; the rest matches).
bool gr_bitmap_3d(vector3 *pos, float angle, float size, gr::Mode mode)
{
    gr_vertex pnt;

    gr_rotate_vertex(&pnt, pos);
    gr_project_vertex(&pnt);
    if (pnt.flags & 1)
        return gr_bitmap_3d_projected(&pnt, angle, size, mode);
    return false;
}

// 0x53f410  Confidence: exact (the binary constructs the vertex, D1605; the rest matches).
bool gr_bitmap_3d_oriented(const vector3 *pos, float angle, float width, float height, gr::Mode mode)
{
    gr_vertex pnt;

    gr_rotate_vertex(&pnt, pos);
    gr_project_vertex(&pnt);
    if (pnt.flags & 1)
        return gr_bitmap_3d_oriented_quad(&pnt, angle, width, height, mode);
    return false;
}

// 0x53f470  Confidence: exact (the order of the vertex declarations). A quad of the given width along
// p0 - p1, facing the viewer; always buffered, sorted by the view z of its centre.
bool gr_line_quad_3d(const vector3 *p0, const vector3 *p1, float width, gr::Mode mode)
{
    vector3 dir = *p0 - *p1;
    dir.normalize_safe();
    vector3 mid = (*p0 + *p1) * 0.5f;
    vector3 view = gr_view_pos - mid;
    view.normalize_safe();
    vector3 side = dir.cross(view);
    side.normalize_safe();

    gr_vertex v0;
    float half = width * 0.5f;
    vector3 a0 = *p0 + side * half;
    gr_rotate_vertex(&v0, &a0);
    gr_vertex v3;
    vector3 a3 = *p0 - side * half;
    gr_rotate_vertex(&v3, &a3);
    gr_vertex v1;
    vector3 a1 = *p1 + side * half;
    gr_rotate_vertex(&v1, &a1);
    gr_vertex v2;
    vector3 a2 = *p1 - side * half;
    gr_rotate_vertex(&v2, &a2);

    vector3 c = ((0.5f * *p0 + 0.5f * *p1) - gr_view_pos).rotated(gr_view_matrix);
    gr_poly_begin(true, c.z, 0, false, 0.0f);
    gr_poly_vertex(&v0);
    gr_poly_vertex(&v1);
    gr_poly_vertex(&v2);
    gr_poly_vertex(&v3);
    gr_poly_set_texture(mode, gr_screen.current_bitmap, gr_screen.current_color.r, gr_screen.current_color.g,
                        gr_screen.current_color.b, gr_screen.current_color.a, 0);
    gr_poly_uv(0.0f, 0.0f);
    gr_poly_uv(0.0f, 1.0f);
    gr_poly_uv(1.0f, 1.0f);
    gr_poly_uv(1.0f, 0.0f);
    gr_poly_end();
    return true;
}

// 0x53f720  Confidence: likely (the eye matrix copies are dead: each axis replaces the matrix; the first
// point is formed from sin / cos of 0.0 computed at run time). Three circles of 5 to 32 segments (the
// projected size) around the axes, and a diameter along each circle's normal.
bool gr_draw_sphere(const vector3 *pos, float radius, gr::Mode mode)
{
    vector3 p;
    gr_vertex first, prev, cur, tmp;
    matrix m, m_saved;
    int nsegs, axis, i;
    float sa, ca, step, angle;

    m = gr_eye_matrix;
    m_saved = gr_eye_matrix;
    gr_rotate_vertex(&first, pos);
    nsegs = (int)gr_project_size(&first, &radius);
    clamp_int(&nsegs, 5, 32);
    sa = (float)sin(0.0);
    ca = (float)cos(0.0);
    for (axis = 0; axis < 3; axis++) {
        switch (axis) {
        case 0:
            m.set_from_unit_forward(vector3(1.0f, 0.0f, 0.0f));
            break;
        case 1:
            m.set_from_unit_forward(vector3(0.0f, 1.0f, 0.0f));
            break;
        case 2:
            m.set_from_unit_forward(vector3(0.0f, 0.0f, 1.0f));
            break;
        }
        step = GR_LINE_2PI / nsegs;
        angle = 0.0f;
        p = *pos + m.rvec * (sa * radius) + m.uvec * (ca * radius);
        gr_rotate_vertex(&first, &p);
        prev = first;
        for (i = 0; i < nsegs; i++) {
            p = *pos + m.rvec * ((float)sin(angle) * radius) + m.uvec * ((float)cos(angle) * radius);
            gr_rotate_vertex(&cur, &p);
            gr_line_3d(&prev, &cur, mode);
            tmp = prev;
            prev = cur;
            cur = tmp;
            angle += step;
        }
        gr_line_3d(&prev, &first, mode);
        p = *pos + m.fvec * radius;
        gr_rotate_vertex(&first, &p);
        p = *pos - m.fvec * radius;
        gr_rotate_vertex(&prev, &p);
        gr_line_3d(&first, &prev, mode);
    }
    return true;
}

// 0x53faf0  Confidence: exact (the binary constructs the eight vertices, D1605; the rest matches). sum.symbols' "arrow" is a wire
// box: the corners pos +- half the size along the three axes of orient, the bottom face, the top face and
// the four sides.
bool gr_draw_arrow_3d(const vector3 *pos, const matrix *orient, const vector3 *size, gr::Mode mode)
{
    vector3 c[8];
    vector3 h = *size * 0.5f;
    int i;

    c[0] = *pos - orient->rvec * h.x - orient->uvec * h.y - orient->fvec * h.z;
    c[1] = *pos + orient->rvec * h.x - orient->uvec * h.y - orient->fvec * h.z;
    c[2] = *pos + orient->rvec * h.x - orient->uvec * h.y + orient->fvec * h.z;
    c[3] = *pos - orient->rvec * h.x - orient->uvec * h.y + orient->fvec * h.z;
    c[4] = *pos - orient->rvec * h.x + orient->uvec * h.y - orient->fvec * h.z;
    c[5] = *pos + orient->rvec * h.x + orient->uvec * h.y - orient->fvec * h.z;
    c[6] = *pos + orient->rvec * h.x + orient->uvec * h.y + orient->fvec * h.z;
    c[7] = *pos - orient->rvec * h.x + orient->uvec * h.y + orient->fvec * h.z;
    gr_vertex v[8];
    for (i = 0; i < 8; i++)
        gr_rotate_vertex(&v[i], &c[i]);
    gr_line_3d(&v[0], &v[1], mode);
    gr_line_3d(&v[1], &v[2], mode);
    gr_line_3d(&v[2], &v[3], mode);
    gr_line_3d(&v[3], &v[0], mode);
    gr_line_3d(&v[4], &v[5], mode);
    gr_line_3d(&v[5], &v[6], mode);
    gr_line_3d(&v[6], &v[7], mode);
    gr_line_3d(&v[7], &v[4], mode);
    gr_line_3d(&v[0], &v[4], mode);
    gr_line_3d(&v[1], &v[5], mode);
    gr_line_3d(&v[2], &v[6], mode);
    gr_line_3d(&v[3], &v[7], mode);
    return true;
}

// 0x53fff0  Confidence: exact (void or gr_line_3d's result; no caller). The points come as six floats
// and are rebuilt with vector3(x, y, z).
void gr_line_3d_points(float x0, float y0, float z0, float x1, float y1, float z1, int r, int g, int b)
{
    vector3 a(x0, y0, z0);
    vector3 e(x1, y1, z1);
    gr_vertex v0, v1;

    gr_rotate_vertex(&v0, &a);
    gr_rotate_vertex(&v1, &e);
    gr_set_color(r, g, b, 255);
    gr_line_3d(&v0, &v1, gr_line_3d_mode);
}

// 0x5400a0  Confidence: exact (as gr_line_3d_points). The shaft, then two head lines from
// mid + dir * |d| / 8 to mid +- side * |d| / 16, mid being the middle of the shaft (so the head sits on the
// middle, not at the end). No caller.
void gr_draw_arrow_3d_points(float x0, float y0, float z0, float x1, float y1, float z1, int r, int g, int b)
{
    vector3 a(x0, y0, z0);
    vector3 e(x1, y1, z1);
    vector3 d = e - a;
    vector3 mid = a + d * 0.5f;
    vector3 side = gr_eye_matrix.fvec.cross(d);
    side.normalize();
    gr_vertex v0, v1;

    gr_rotate_vertex(&v0, &a);
    gr_rotate_vertex(&v1, &e);
    gr_set_color(r, g, b, 255);
    gr_line_3d(&v0, &v1, gr_line_3d_mode);
    vector3 p, q;
    float len = d.len() * 0.125f;
    float half = len * 0.5f;
    d.normalize();
    q = mid + d * len;
    p = mid + side * half;
    gr_rotate_vertex(&v0, &q);
    gr_rotate_vertex(&v1, &p);
    gr_set_color(r, g, b, 255);
    gr_line_3d(&v0, &v1, gr_line_3d_mode);
    p = mid - side * half;
    gr_rotate_vertex(&v1, &p);
    gr_set_color(r, g, b, 255);
    gr_line_3d(&v0, &v1, gr_line_3d_mode);
}

// 0x540330  Confidence: exact (VC6 byte match, match22; the head length in its own local; was likely) (q is constructed and never used; d is normalised and not used again). The
// shaft a - b and a head from b back to a + 0.9 (b - a) +- side * |b - a| / 16; nothing when both ends
// project to the same view x / y.
void gr_draw_arrow_3d_between(float x0, float y0, float z0, float x1, float y1, float z1, int r, int g, int b)
{
    vector3 a(x0, y0, z0);
    vector3 e(x1, y1, z1);
    vector3 d = e - a;
    vector3 head = a + d * 0.9f;
    vector3 side = gr_eye_matrix.fvec.cross(d);
    side.normalize();
    gr_vertex v0, v1;

    gr_rotate_vertex(&v0, &a);
    gr_rotate_vertex(&v1, &e);
    if (v0.x == v1.x && v0.y == v1.y)
        return;
    gr_set_color(r, g, b, 255);
    gr_line_3d(&v0, &v1, gr_line_3d_mode);
    vector3 p, q;
    float head_len = d.len() * 0.125f;
    float len = head_len * 0.5f;
    d.normalize();
    p = head + side * len;
    gr_rotate_vertex(&v0, &p);
    gr_set_color(r, g, b, 255);
    gr_line_3d(&v0, &v1, gr_line_3d_mode);
    p = head - side * len;
    gr_rotate_vertex(&v0, &p);
    gr_set_color(r, g, b, 255);
    gr_line_3d(&v0, &v1, gr_line_3d_mode);
}

// 0x540590  Confidence: exact. No caller.
void gr_draw_axes_3d(const vector3 *pos, const matrix *orient, float len)
{
    vector3 p = *pos;
    vector3 q;
    ubyte r = gr_screen.current_color.r;
    ubyte g = gr_screen.current_color.g;
    ubyte b = gr_screen.current_color.b;
    ubyte a = gr_screen.current_color.a;

    q = p + orient->rvec * len;
    gr_set_color(255, 0, 0, 255);
    gr_draw_arrow_3d_between(p.x, p.y, p.z, q.x, q.y, q.z, gr_screen.current_color.r, gr_screen.current_color.g,
                             gr_screen.current_color.b);
    q = p + orient->uvec * len;
    gr_set_color(0, 255, 0, 255);
    gr_draw_arrow_3d_between(p.x, p.y, p.z, q.x, q.y, q.z, gr_screen.current_color.r, gr_screen.current_color.g,
                             gr_screen.current_color.b);
    q = p + orient->fvec * len;
    gr_set_color(0, 0, 255, 255);
    gr_draw_arrow_3d_between(p.x, p.y, p.z, q.x, q.y, q.z, gr_screen.current_color.r, gr_screen.current_color.g,
                             gr_screen.current_color.b);
    gr_set_color(r, g, b, a);
}

// Compiler-generated, at the end of the object (not source here):
// 0x540790 gr__Vertex__assign: the implicit copy assignment of gr::Vertex (vector3::operator= on +0, then
// the other fields one by one), called by gr_draw_sphere. Confidence: likely (D1605: gr.h's gr_vertex has
// plain floats, so the assignments above copy inline).
