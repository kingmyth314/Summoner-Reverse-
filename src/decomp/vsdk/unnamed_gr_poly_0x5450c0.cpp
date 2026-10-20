// unnamed_gr_poly_0x5450c0.cpp - Sum.exe v1.40, code 0x5450c0-0x54614a: the polygon builder of the renderer
// (vertex-by-vertex polygons with up to four texture passes, clipped and drawn through gr_tmapper_multipass,
// or recorded into a command buffer that gr_poly_buffer_flush sorts far to near and replays; vsdk\gr in
// sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp69 NOTES.md): the vfile array object ends at 0x5450bb; this one opens with its two
// initialisers (table 0x5889fc / 0x588a00), its regular functions end with gr_poly_buffer_get_data
// (0x5460e0), then the COMDATs: the implicit constructor of gr_tmapper_pass (0x546110) and the two inline
// clip pool helpers (0x546120 / 0x546130, vsdk/gr/unnamed_gr_poly_0x5450c0.h). The sound emitter object
// follows (0x546150). Data: .data 0x5aca38-0x5aca47, .bss 0x2ee45d8-0x2f076eb (after the skeleton
// object's 0x2ee45a8 block; the emitters start at 0x2f076f0).
//
// Like the other vsdk objects this one calls the header inlines out of line (gr::Mode, float_min /
// float_max, gr_poly_free_clip_verts, gr_poly_alloc_clip_vertex), so it was built without inline
// expansion.
//
// Bug (NOTES.md): gr_poly_rgb with the tint on stores the green result into red and leaves green
// untinted (gr_poly_set_texture does all three).

#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/unnamed_clip_0x5590b0.h"
#include "vsdk/gr/unnamed_poly_split_0x55f110.h"
#include "vsdk/gr/unnamed_gr_poly_0x5450c0.h"

// ---- .bss, in address order ------------------------------------------------------------------------------
// 0x5450c0 init_gr_vertex_buf_2ee6338 (jmp 0x5450d0: `vector constructor iterator` over gr_poly_clip_vertices
// with gr__Vertex__Vertex 0x5081c0): compiler-generated; the array has an initialiser only because
// gr_vertex has a default constructor in the original (D1605, still open; as D4703). Confidence: likely.
// 0x5450f0 init_gr_mode_slots_2ee45d8 (jmp 0x545100: `vector constructor iterator` over the four pass
// records with their implicit constructor 0x546110): compiler-generated. Confidence: likely. The table
// order (0x5889fc the vertices, 0x588a00 the passes) is the definition order.
gr_vertex gr_poly_clip_vertices[GR_POLY_MAX_CLIP_VERTS];   // 0x2ee6338
gr_tmapper_pass gr_poly_texture_slots[GR_POLY_MAX_PASSES];   // 0x2ee45d8
bool gr_poly_active;                                    // 0x2ee4628
gr_poly_vertex_rec gr_poly_clip_records[GR_POLY_MAX_CLIP_VERTS];   // 0x2ee4630
ubyte *gr_poly_buffer_polys[GR_POLY_BUFFER_MAX_POLYS + 1];   // 0x2ee5330
float gr_poly_depth;                                    // 0x2ee6330
gr_poly_vertex_rec gr_poly_vertices[GR_POLY_MAX_VERTS]; // 0x2ee6f38
ubyte gr_poly_buffer[GR_POLY_BUFFER_SIZE];              // 0x2ee75b8
bool gr_poly_multitexture;                              // 0x2f075b8
gr_poly_vertex_rec *gr_poly_clip_vertex_ptrs[GR_POLY_MAX_CLIP_VERTS];   // 0x2f075bc
bool gr_poly_fixed_depth;                               // 0x2f076bc
int gr_poly_clip_vertex_count;                          // 0x2f076c0
bool gr_poly_clip_verts_ready;                          // 0x2f076c4 (gr_setup_3d)
bool gr_poly_in_progress;                               // 0x2f076c5
int gr_poly_num_vertices;                               // 0x2f076c8
int gr_poly_num_textures;                               // 0x2f076cc
gr_tmapper_pass *gr_poly_cur_texture;                   // 0x2f076d0
int gr_poly_cur_texture_index;                          // 0x2f076d4
gr_poly_vertex_rec *gr_poly_uv_cursor;                  // 0x2f076d8
gr_poly_vertex_rec *gr_poly_rgb_cursor;                 // 0x2f076dc
gr_poly_vertex_rec *gr_poly_alpha_cursor;               // 0x2f076e0
int gr_poly_arg2;                                       // 0x2f076e4
int gr_poly_buffer_count;                               // 0x2f076e8

// ---- .data -----------------------------------------------------------------------------------------------
ubyte *gr_poly_buffer_limit = &gr_poly_buffer[GR_POLY_BUFFER_SIZE - 0x200];   // 0x5aca38
ubyte *gr_poly_buffer_last = &gr_poly_buffer[GR_POLY_BUFFER_SIZE - 1];       // 0x5aca3c
ubyte *gr_poly_buffer_ptr = gr_poly_buffer;            // 0x5aca40
bool gr_poly_buffer_first_overflow = true;              // 0x5aca44

// 0x545120  Confidence: exact.
void gr_poly_reset_clip_verts()
{
    int i;

    gr_poly_clip_vertex_count = 0;
    for (i = 0; i < GR_POLY_MAX_CLIP_VERTS; i++) {
        gr_poly_vertex_rec *r = &gr_poly_clip_records[i];
        gr_poly_clip_vertex_ptrs[i] = r;
        r->v = &gr_poly_clip_vertices[i];
    }
}

// 0x545150  Confidence: exact (the cleared flag before the flush is what is left of a first-time
// message).
void gr_poly_begin(bool buffered, float z, int a2, bool fixed_depth, float depth)
{
    if (buffered) {
        if (gr_poly_buffer_ptr >= gr_poly_buffer_limit || gr_poly_buffer_count >= GR_POLY_BUFFER_MAX_POLYS) {
            if (gr_poly_buffer_first_overflow)
                gr_poly_buffer_first_overflow = false;
            gr_poly_buffer_flush();
        }
        gr_poly_active = buffered;
        gr_poly_buffer_polys[gr_poly_buffer_count++] = gr_poly_buffer_ptr;
        if (fixed_depth) {
            gr_poly_buffer_put_byte(GR_POLY_OP_BEGIN_DEPTH);
            gr_poly_buffer_put_byte(a2);
            gr_poly_buffer_skip(2);
            gr_poly_buffer_put_float(z);
            gr_poly_buffer_put_float(depth);
        } else {
            gr_poly_buffer_put_byte(GR_POLY_OP_BEGIN);
            gr_poly_buffer_put_byte(a2);
            gr_poly_buffer_skip(2);
            gr_poly_buffer_put_float(z);
        }
        return;
    }
    gr_poly_in_progress = true;
    gr_poly_num_vertices = 0;
    gr_poly_num_textures = 0;
    gr_poly_arg2 = a2;
    gr_poly_uv_cursor = gr_poly_rgb_cursor = gr_poly_alpha_cursor = gr_poly_vertices;
    gr_poly_multitexture = false;
    gr_poly_fixed_depth = fixed_depth;
    gr_poly_depth = depth;
}

// 0x545260  Confidence: exact.
void gr_poly_buffer_skip(int n)
{
    gr_poly_buffer_ptr += n;
}

// 0x545280  Confidence: exact.
void gr_poly_buffer_put_byte(ubyte b)
{
    *gr_poly_buffer_ptr++ = b;
}

// 0x5452a0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21).
void gr_poly_buffer_put_float(float f)
{
    *(float *)gr_poly_buffer_ptr = f;
    gr_poly_buffer_ptr += 4;
}

// 0x5452c0  Confidence: exact. Buffered: the clip codes and the view-space position only.
void gr_poly_vertex(gr_vertex *v)
{
    if (gr_poly_active) {
        gr_poly_buffer_put_byte(GR_POLY_OP_VERTEX);
        gr_poly_buffer_put_byte(v->codes);
        gr_poly_buffer_skip(2);
        gr_poly_buffer_put_vector((vector3 *)&v->x);
        return;
    }
    gr_poly_vertex_rec *rec = &gr_poly_vertices[gr_poly_num_vertices++];
    rec->v = v;
}

// 0x545310  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21; was likely; a vector3 copy through a walking pointer, as gr_poly_buffer_get_vector,
// which matches; the cursor store before the three dwords is what /Ow gives, D8000).
void gr_poly_buffer_put_vector(const vector3 *v)
{
    vector3 *p = (vector3 *)gr_poly_buffer_ptr;

    *p++ = *v;
    gr_poly_buffer_ptr = (ubyte *)p;
}

// m-1020 (D11600): float_max / float_min are called out of line here (0x50d960, 0x51faf0), as in gr_poly_rgb.
#pragma inline_depth(0)
// 0x545340  Confidence: exact (VC6 byte match, m-1020, D11600: inline_depth(0), one float local per
// channel, the size locals in their block so that the frame is 8 bytes as the binary). The tint is applied to the colour (grey = r + g + b times each tint
// factor, as gr_glide_apply_tint); a bitmap larger than the largest texture marks the pass sectioned.
void gr_poly_set_texture(gr::Mode mode, int bm, ubyte r, ubyte g, ubyte b, ubyte a, int stage)
{
    float sum;

    if (gr_screen.tint_enabled) {
        sum = (float)(r + g + b);
        float tr = gr_screen.tint_r * sum;
        tr = float_max(tr, 0.0f);
        r = (ubyte)float_min(tr, 255.0f);
        float tg = gr_screen.tint_g * sum;
        tg = float_max(tg, 0.0f);
        g = (ubyte)float_min(tg, 255.0f);
        float tb = gr_screen.tint_b * sum;
        tb = float_max(tb, 0.0f);
        b = (ubyte)float_min(tb, 255.0f);
    }
    if (gr_poly_active) {
        gr_poly_buffer_put_byte(GR_POLY_OP_TEXTURE);
        gr_poly_buffer_put_byte(stage);
        gr_poly_buffer_skip(2);
        gr_poly_buffer_put_data(&mode, 4);
        gr_poly_buffer_put_int(bm);
        gr_poly_buffer_put_byte(r);
        gr_poly_buffer_put_byte(g);
        gr_poly_buffer_put_byte(b);
        gr_poly_buffer_put_byte(a);
        return;
    }
    gr_poly_cur_texture_index = gr_poly_num_textures;
    gr_poly_cur_texture = &gr_poly_texture_slots[gr_poly_num_textures++];
    gr_poly_cur_texture->mode = mode;
    gr_poly_cur_texture->bitmap = bm;
    gr_poly_cur_texture->sectioned = false;
    if (bm > -1) {
        int w, h;
        bm_get_dimensions(bm, &w, &h);
        if (w > gr_screen.max_bitmap_w || h > gr_screen.max_bitmap_h) {
            gr_poly_multitexture = true;
            gr_poly_cur_texture->sectioned = true;
        }
    }
    gr_poly_cur_texture->enabled = false;
    gr_poly_cur_texture->vertex_color = false;
    gr_poly_cur_texture->vertex_alpha = false;
    gr_poly_cur_texture->stage = (ubyte)stage;
    gr_poly_cur_texture->r = r;
    gr_poly_cur_texture->g = g;
    gr_poly_cur_texture->b = b;
    gr_poly_cur_texture->a = a;
    gr_poly_uv_cursor = gr_poly_rgb_cursor = gr_poly_alpha_cursor = gr_poly_vertices;
}
#pragma inline_depth()

// 0x545550  Confidence: exact.
void gr_poly_buffer_put_int(int i)
{
    *(int *)gr_poly_buffer_ptr = i;
    gr_poly_buffer_ptr += 4;
}

// 0x545570  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21).
void gr_poly_buffer_put_data(const void *src, int n)
{
    memcpy(gr_poly_buffer_ptr, src, n);
    gr_poly_buffer_ptr += n;
}

// 0x5455a0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21).
void gr_poly_uv(float u, float v)
{
    gr_poly_vertex_rec *rec;

    if (gr_poly_active) {
        gr_poly_buffer_put_byte(GR_POLY_OP_UV);
        gr_poly_buffer_skip(3);
        gr_poly_buffer_put_float(u);
        gr_poly_buffer_put_float(v);
        return;
    }
    gr_poly_cur_texture->enabled = true;
    gr_poly_uv_cursor->pass[gr_poly_cur_texture_index].u = u;
    gr_poly_uv_cursor->pass[gr_poly_cur_texture_index].v = v;
    gr_poly_uv_cursor++;
}

// m-1020 (D11600): float_max / float_min are called out of line here, as in the binary (0x50d960,
// 0x51faf0); this unit otherwise expands its inlines, so inline expansion is off for this function only.
#pragma inline_depth(0)
// 0x545610  Confidence: exact (VC6 byte match, m-1020, D11600: inline_depth(0), each clamp's inner
// call and product through locals, the pass record through a pointer and the cursor advanced last).
// The copy-and-paste slip that the dead first result shows: the red tint is computed and dropped
// (`fstp st(0)` instead of _ftol), and the green result is stored into r.
void gr_poly_rgb(ubyte r, ubyte g, ubyte b)
{
    float sum;

    if (gr_screen.tint_enabled) {
        sum = (float)(r + g + b);
        float prod = gr_screen.tint_r * sum;
        float t = float_max(prod, 0.0f);
        r = (ubyte)float_min(t, 255.0f);
        prod = gr_screen.tint_g * sum;
        t = float_max(prod, 0.0f);
        r = (ubyte)float_min(t, 255.0f);
        prod = gr_screen.tint_b * sum;
        t = float_max(prod, 0.0f);
        b = (ubyte)float_min(t, 255.0f);
    }
    if (gr_poly_active) {
        gr_poly_buffer_put_byte(GR_POLY_OP_RGB);
        gr_poly_buffer_put_byte(r);
        gr_poly_buffer_put_byte(g);
        gr_poly_buffer_put_byte(b);
        return;
    }
    gr_poly_cur_texture->vertex_color = true;
    gr_pass_vertex_data *p = &gr_poly_rgb_cursor->pass[gr_poly_cur_texture_index];
    p->r = r;
    p->g = g;
    p->b = b;
    gr_poly_rgb_cursor++;
}
#pragma inline_depth()

// 0x545760  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21).
void gr_poly_alpha(ubyte a)
{
    if (gr_poly_active) {
        gr_poly_buffer_put_byte(GR_POLY_OP_ALPHA);
        gr_poly_buffer_put_byte(a);
        gr_poly_buffer_skip(2);
        return;
    }
    gr_poly_cur_texture->vertex_alpha = true;
    gr_poly_alpha_cursor->pass[gr_poly_cur_texture_index].a = a;
    gr_poly_alpha_cursor++;
}

// 0x5457b0  Confidence: exact. Rejects the polygon when one clip plane has every vertex outside.
bool gr_poly_end()
{
    gr_multipass_vertex *list[GR_POLY_MAX_VERTS];
    gr_multipass_vertex *tmp[GR_POLY_MAX_VERTS];
    ubyte codes[2];
    int i, n;

    if (gr_poly_active) {
        gr_poly_buffer_put_byte(GR_POLY_OP_END);
        gr_poly_buffer_skip(3);
        gr_poly_active = false;
        return true;
    }
    n = gr_poly_num_vertices;
    gr_poly_in_progress = false;
    codes[0] = 0;
    codes[1] = 0xff;
    for (i = 0; i < n; i++)
        list[i] = (gr_multipass_vertex *)&gr_poly_vertices[i];
    for (i = 0; i < n; i++) {
        codes[1] &= list[i]->v->codes;
        codes[0] |= list[i]->v->codes;
    }
    if (codes[1])
        return false;
    return gr_poly_clip_and_draw(codes, list, tmp);
}

// m-1020b w8 (D13200): the binary calls the clip-vertex helpers out of line here and in gr_poly_clip_edge.
#pragma inline_depth(0)
// 0x545870  Confidence: likely (the exits; m-1020b w8, D13200: list copied to a local, an element pointer per
// vertex; only the shrink-wrapped prologue differs). A vertex behind the viewer drops the polygon; fixed depth sets
// every vertex's z and 1/z unless a pass is sectioned.
bool gr_poly_clip_and_draw(ubyte *codes, gr_multipass_vertex **list, gr_multipass_vertex **tmp)
{
    gr_multipass_vertex **l = list;
    int nv = gr_poly_num_vertices;
    int i;
    gr_vertex *v;

    if (gr_poly_num_textures == 0)
        return false;
    if (codes[0]) {
        l = gr_poly_clip_planes(l, tmp, &nv, codes);
        if (nv == 0 || codes[1])
            goto done;
    }
    for (i = 0; i < nv; i++) {
        gr_multipass_vertex *m = l[i];
        if (!(m->v->flags & 1))
            gr_project_vertex(m->v);
        v = m->v;
        if (v->flags & 2)
            goto done;
        if (gr_poly_fixed_depth && !gr_poly_multitexture) {
            v->z = gr_poly_depth;
            v->sw = 1.0f / gr_poly_depth;
        }
    }
    if (gr_poly_multitexture) {
        gr_poly_split_sections_draw(nv, l, gr_poly_num_textures, gr_poly_texture_slots);
        gr_poly_reset_clip_verts();
        gr_poly_free_clip_verts();
        return true;
    }
    gr_tmapper_multipass(nv, l, gr_poly_num_textures, gr_poly_texture_slots);
done:
    gr_poly_free_clip_verts();
    return true;
}
#pragma inline_depth()

// 0x545970  Confidence: exact. As gr_clip_polygon, but up to plane bit 0x20 (the behind plane included).
gr_multipass_vertex **gr_poly_clip_planes(gr_multipass_vertex **src, gr_multipass_vertex **tmp, int *nv, ubyte *codes)
{
    int bit;
    gr_multipass_vertex **t;

    for (bit = 1; bit <= 0x20; bit <<= 1) {
        if (codes[0] & bit) {
            *nv = gr_poly_clip_plane(bit, src, tmp, nv, codes);
            if (codes[1] != 0)
                return tmp;
            t = src;
            src = tmp;
            tmp = t;
        }
    }
    return src;
}

// 0x5459d0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21). As gr_clip_plane on the multipass vertices.
int gr_poly_clip_plane(int plane, gr_multipass_vertex **src, gr_multipass_vertex **dst, int *nv, ubyte *codes)
{
    gr_multipass_vertex **start = dst;
    gr_multipass_vertex *t;
    int i, n;

    n = *nv;
    src[n] = src[0];
    src[n + 1] = src[1];
    codes[1] = 0xff;
    codes[0] = 0;
    for (i = 1; i <= *nv; i++) {
        if (src[i]->v->codes & plane) {
            if (!(src[i - 1]->v->codes & plane)) {
                t = gr_poly_clip_edge(plane, src[i - 1], src[i]);
                *dst = t;
                codes[0] |= t->v->codes;
                codes[1] &= t->v->codes;
                dst++;
            }
            if (!(src[i + 1]->v->codes & plane)) {
                t = gr_poly_clip_edge(plane, src[i + 1], src[i]);
                *dst = t;
                codes[0] |= t->v->codes;
                codes[1] &= t->v->codes;
                dst++;
            }
        } else {
            *dst++ = src[i];
            codes[0] |= src[i]->v->codes;
            codes[1] &= src[i]->v->codes;
        }
    }
    return dst - start;
}

#pragma inline_depth(0)
// 0x545ac0  Confidence: likely (m-1020b w8, D13200: three uv addresses differ in base / index order only; as gr_clip_edge; the far crossing stores z last). The pass values are
// interpolated for the passes that use them (uv, colour, alpha); the alpha is 0 at the far plane when
// the outside vertex's is.
gr_multipass_vertex *gr_poly_clip_edge(int plane, gr_multipass_vertex *on, gr_multipass_vertex *off)
{
    gr_multipass_vertex *tmp;
    gr_vertex *a, *b, *tv;
    gr_tmapper_pass *pass;
    float t, d, p0, p1;
    float r, g, bl, al, al_off;
    int negate, far_plane, i;

    tmp = gr_poly_alloc_clip_vertex();
    a = on->v;
    b = off->v;
    tv = tmp->v;
    far_plane = plane & GR_CLIP_FAR;
    if (far_plane) {
        d = b->z - a->z;
        if (d == 0.0f)
            t = 1.0f;
        else
            t = (gr_far_clip_z - a->z) / d;
        tv->x = (b->x - a->x) * t + a->x;
        tv->y = (b->y - a->y) * t + a->y;
        tv->z = gr_far_clip_z;
    } else {
        if (plane & (GR_CLIP_LEFT | GR_CLIP_RIGHT)) {
            p0 = a->x;
            p1 = b->x;
        } else {
            p0 = a->y;
            p1 = b->y;
        }
        negate = plane & (GR_CLIP_LEFT | GR_CLIP_BOTTOM);
        if (negate) {
            p0 = -p0;
            p1 = -p1;
        }
        t = (p0 - a->z) / ((p0 - a->z) - p1 + b->z);
        tv->x = (b->x - a->x) * t + a->x;
        tv->y = (b->y - a->y) * t + a->y;
        if (plane & (GR_CLIP_BOTTOM | GR_CLIP_TOP))
            tv->z = tv->y;
        else
            tv->z = tv->x;
        if (negate)
            tv->z = -tv->z;
    }
    gr_code_vertex(tv);
    for (i = 0; i < gr_poly_num_textures; i++) {
        pass = &gr_poly_texture_slots[i];
        if (pass->enabled) {
            tmp->pass[i].u = (off->pass[i].u - on->pass[i].u) * t + on->pass[i].u;
            tmp->pass[i].v = (off->pass[i].v - on->pass[i].v) * t + on->pass[i].v;
        }
        if (pass->vertex_color) {
            r = on->pass[i].r;
            g = on->pass[i].g;
            bl = on->pass[i].b;
            tmp->pass[i].r = (ubyte)(int)((off->pass[i].r - r) * t + r);
            tmp->pass[i].g = (ubyte)(int)((off->pass[i].g - g) * t + g);
            tmp->pass[i].b = (ubyte)(int)((off->pass[i].b - bl) * t + bl);
        }
        if (pass->vertex_alpha) {
            al = on->pass[i].a;
            al_off = off->pass[i].a;
            if (far_plane && al_off == 0.0f)
                tmp->pass[i].a = 0;
            else
                tmp->pass[i].a = (ubyte)(int)((al_off - al) * t + al);
        }
    }
    return tmp;
}
#pragma inline_depth()

// m-1020b w8 (D13200): the binary builds verts with `vector constructor iterator` (0x404880) and gr_vertex's
// implicit constructor, so inline expansion is off for this function too (as gr.cpp's /Ob0, D12103).
#pragma inline_depth(0)
// 0x545d70  Confidence: exact (VC6 byte match, m-1020b w8, D13200: inline_depth(0), nv cleared before the
// array, one a2 / z for both begin opcodes (the binary cross-jumps their gr_poly_begin calls), the end opcode
// leaves the endless replay loop by a goto, as the binary's other opcodes jump straight back). Shell sort of the
// recorded polygons by their z, largest first, then each one is replayed into the immediate calls with
// the tint off (the colours were tinted when recorded).
void gr_poly_buffer_flush()
{
    bool saved_tint;
    int gap, i, j, k, nv;
    ubyte *t;
    ubyte a2;
    float z;

    if (gr_poly_buffer_ptr == gr_poly_buffer)
        return;
    saved_tint = gr_screen.tint_enabled;
    gr_screen.tint_enabled = false;
    for (gap = gr_poly_buffer_count / 2; gap > 0; gap /= 2) {
        for (i = gap; i < gr_poly_buffer_count; i++) {
            for (j = i - gap; j >= 0; j -= gap) {
                if (!(*(float *)(gr_poly_buffer_polys[j] + 4) < *(float *)(gr_poly_buffer_polys[j + gap] + 4)))
                    break;
                t = gr_poly_buffer_polys[j];
                gr_poly_buffer_polys[j] = gr_poly_buffer_polys[j + gap];
                gr_poly_buffer_polys[j + gap] = t;
            }
        }
    }
    nv = 0;
    gr_vertex verts[25];
    for (k = 0; k < gr_poly_buffer_count; k++) {
        gr_poly_buffer_ptr = gr_poly_buffer_polys[k];
        for (;;) {
            switch (gr_poly_buffer_get_byte()) {
            case GR_POLY_OP_BEGIN:
                a2 = gr_poly_buffer_get_byte();
                gr_poly_buffer_skip(2);
                z = gr_poly_buffer_get_float();
                nv = 0;
                gr_poly_begin(false, z, a2, false, 0.0f);
                break;
            case GR_POLY_OP_BEGIN_DEPTH: {
                a2 = gr_poly_buffer_get_byte();
                gr_poly_buffer_skip(2);
                z = gr_poly_buffer_get_float();
                float depth = gr_poly_buffer_get_float();
                nv = 0;
                gr_poly_begin(false, z, a2, true, depth);
                break;
            }
            case GR_POLY_OP_VERTEX: {
                gr_vertex *v = &verts[nv++];
                v->codes = gr_poly_buffer_get_byte();
                gr_poly_buffer_skip(2);
                gr_poly_buffer_get_vector((vector3 *)&v->x);
                v->flags = 0;
                gr_poly_vertex(v);
                break;
            }
            case GR_POLY_OP_TEXTURE: {
                gr::Mode mode;
                ubyte stage = gr_poly_buffer_get_byte();
                gr_poly_buffer_skip(2);
                gr_poly_buffer_get_data(&mode, 4);
                int bm = gr_poly_buffer_get_int();
                ubyte r = gr_poly_buffer_get_byte();
                ubyte g = gr_poly_buffer_get_byte();
                ubyte b = gr_poly_buffer_get_byte();
                ubyte a = gr_poly_buffer_get_byte();
                gr_poly_set_texture(mode, bm, r, g, b, a, stage);
                break;
            }
            case GR_POLY_OP_UV: {
                gr_poly_buffer_skip(3);
                float u = gr_poly_buffer_get_float();
                float v = gr_poly_buffer_get_float();
                gr_poly_uv(u, v);
                break;
            }
            case GR_POLY_OP_RGB: {
                ubyte r = gr_poly_buffer_get_byte();
                ubyte g = gr_poly_buffer_get_byte();
                ubyte b = gr_poly_buffer_get_byte();
                gr_poly_rgb(r, g, b);
                break;
            }
            case GR_POLY_OP_ALPHA: {
                ubyte a = gr_poly_buffer_get_byte();
                gr_poly_buffer_skip(2);
                gr_poly_alpha(a);
                break;
            }
            case GR_POLY_OP_END:
                gr_poly_buffer_skip(3);
                gr_poly_end();
                goto next;
            }
        }
    next:;
    }
    gr_screen.tint_enabled = saved_tint;
    gr_poly_buffer_ptr = gr_poly_buffer;
    gr_poly_buffer_count = 0;
}
#pragma inline_depth()

// 0x546070  Confidence: exact.
ubyte gr_poly_buffer_get_byte()
{
    return *gr_poly_buffer_ptr++;
}

// 0x546080  Confidence: exact.
float gr_poly_buffer_get_float()
{
    float f = *(float *)gr_poly_buffer_ptr;

    gr_poly_buffer_ptr += 4;
    return f;
}

// 0x546090  Confidence: exact.
int gr_poly_buffer_get_int()
{
    int i = *(int *)gr_poly_buffer_ptr;

    gr_poly_buffer_ptr += 4;
    return i;
}

// 0x5460b0  Confidence: exact (VC6 byte match, cl 8966; a vector3 copy through a walking pointer, D7002).
void gr_poly_buffer_get_vector(vector3 *out)
{
    vector3 *p = (vector3 *)gr_poly_buffer_ptr;

    *out = *p++;
    gr_poly_buffer_ptr = (ubyte *)p;
}

// 0x5460e0  Confidence: exact (VC6 byte match, cl 8966, /Ox /Ow (D8000), match21).
void gr_poly_buffer_get_data(void *dst, int n)
{
    memcpy(dst, gr_poly_buffer_ptr, n);
    gr_poly_buffer_ptr += n;
}

// Compiler-generated and header inlines at the end of the object (not source here):
// 0x546110 gr_mode_slot__ctor: the implicit constructor of gr_tmapper_pass (gr.h; gr::Mode() on +0).
// Confidence: likely.
// 0x546120 gr_poly_free_clip_verts (vsdk/gr/unnamed_gr_poly_0x5450c0.h). Confidence: exact.
// 0x546130 gr_poly_alloc_clip_vertex (vsdk/gr/unnamed_gr_poly_0x5450c0.h). Confidence: exact.
