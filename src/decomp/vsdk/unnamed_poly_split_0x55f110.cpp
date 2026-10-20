// unnamed_poly_split_0x55f110.cpp - Sum.exe v1.40, code 0x55f110-0x55fc6f: multipass polygons cut along
// the section lines of bitmaps larger than the card's texture size, and the section lookup the back ends
// use (vsdk\gr in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the slider unit (0x55f10b), before the vfx dummy code (0x55fc70).
// .bss 0x2f8f750-0x2f91a5b (after the ds3d object's 0x2f8f749), no .data. Built without inline expansion
// (float_min / float_max, imax and the vector3 operators are calls).

#include <math.h>
#include "vsdk/gr/unnamed_poly_split_0x55f110.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/math/vector.h"

// ---- .bss -------------------------------------------------------------------------------------------
gr_poly_split_rec *gr_poly_split_free_list;          // 0x2f8f750
gr_poly_split_rec gr_poly_split_pool[GR_POLY_SPLIT_POOL];   // 0x2f8f758
gr_poly_split_rec *gr_poly_split_active_list;        // 0x2f91a58

// 0x55f110  Confidence: likely (the list code is vsdk.h's plist macros; the pool's free list is built in a
// local first). A piece is cut while a section line crosses it; the scan restarts nowhere, it continues
// with the second half.
void gr_poly_split_sections_draw(int nv, gr_multipass_vertex **verts, int npasses, gr_tmapper_pass *passes)
{
    gr_poly_split_rec *head;
    gr_poly_split_rec *r;
    gr_poly_split_rec *p;
    gr_poly_split_rec *a;
    gr_poly_split_rec *b;
    int bw, bh;
    float step;
    float lo, hi, c;
    float line;
    int first, last;
    int i, j, k, axis;

    head = NULL;
    for (r = gr_poly_split_pool; r < gr_poly_split_pool + GR_POLY_SPLIT_POOL; r++)
        plist_append(head, r);
    gr_poly_split_free_list = head;
    p = gr_poly_split_free_list;
    plist_remove(gr_poly_split_free_list, p);
    gr_poly_split_active_list = NULL;
    plist_append(gr_poly_split_active_list, p);
    p->nv = nv;
    for (i = 0; i < nv; i++)
        p->verts[i] = verts[i];
    for (i = 0; i < npasses; i++) {
        if (!passes[i].sectioned)
            continue;
        bm_get_dimensions(passes[i].bitmap, &bw, &bh);
        for (axis = 0; axis < 2; axis++) {
            if (axis == 0) {
                step = 256.0f / bw;
                if (!(step < 1.0f))
                    continue;
                c = verts[0]->pass[i].u;
            } else {
                step = 256.0f / bh;
                if (!(step < 1.0f))
                    continue;
                c = verts[0]->pass[i].v;
            }
            hi = c;
            lo = c;
            for (j = 1; j < nv; j++) {
                if (axis == 0)
                    c = verts[j]->pass[i].u;
                else
                    c = verts[j]->pass[i].v;
                lo = float_min(lo, c);
                hi = float_max(hi, c);
            }
            first = (int)ceil(lo / step);
            last = (int)floor(hi / step);
            for (k = first; k <= last; k++) {
                line = k * step;
                p = gr_poly_split_active_list;
                while (p != NULL) {
                    a = gr_poly_split_free_list;
                    b = a->next != a ? a->next : NULL;
                    if (a == NULL || b == NULL)
                        return;
                    if (gr_poly_split_at_tex_line(p, a, b, npasses, passes, i, axis, line)) {
                        plist_remove(gr_poly_split_free_list, a);
                        plist_remove(gr_poly_split_free_list, b);
                        a->next = p->next;
                        a->prev = p;
                        p->next->prev = a;
                        p->next = a;
                        b->next = a;
                        b->prev = p;
                        p->next->prev = b;
                        p->next = b;
                        plist_remove(gr_poly_split_active_list, p);
                        p = a;
                    } else {
                        p = p->next;
                        if (p == gr_poly_split_active_list)
                            break;
                    }
                }
            }
        }
    }
    p = gr_poly_split_active_list;
    while (p != NULL) {
        gr_tmapper_multipass(p->nv, p->verts, npasses, passes);
        p = p->next;
        if (p == gr_poly_split_active_list)
            break;
    }
}

// 0x55f580  Confidence: likely (the edge bookkeeping). Two crossed edges, or one crossed edge and the
// vertex nearest the line (when the line runs through a corner), give the cut; a goes from the first
// edge's end to the second edge, b the other way round, each closed by the two new vertices.
bool gr_poly_split_at_tex_line(gr_poly_split_rec *p, gr_poly_split_rec *a, gr_poly_split_rec *b, int npasses,
                               gr_tmapper_pass *passes, int pass, int axis, float line)
{
    int crossings = 0;
    int e1a = 0, e1b = 0;
    int e2a = 0, e2b = 0;
    int j, k;
    int best;
    float bestd;
    float d;
    float c0, c1;
    float t1, t2;
    gr_multipass_vertex *n1;
    gr_multipass_vertex *n2;

    for (j = 0; j < p->nv; j++) {
        k = (j + 1) % p->nv;
        if (axis == 0) {
            c0 = p->verts[j]->pass[pass].u;
            c1 = p->verts[k]->pass[pass].u;
        } else {
            c0 = p->verts[j]->pass[pass].v;
            c1 = p->verts[k]->pass[pass].v;
        }
        if (float_strictly_between(c0, c1, line)) {
            if (crossings == 0) {
                e1a = j;
                e1b = k;
            } else if (crossings == 1) {
                e2a = j;
                e2b = k;
            }
            crossings++;
        }
    }
    if (crossings == 0)
        return false;
    if (crossings < 2) {
        best = -1;
        bestd = 3.4028235e38f;
        for (j = 0; j < p->nv; j++) {
            if (axis == 0)
                d = (float)fabs(p->verts[j]->pass[pass].u - line);
            else
                d = (float)fabs(p->verts[j]->pass[pass].v - line);
            if (d < bestd) {
                bestd = d;
                best = j;
            }
        }
        k = (best + 1) % p->nv;
        if (best == e1a || best == e1b || k == e1a || k == e1b)
            return false;
        e2a = best;
        e2b = k;
    }
    if (axis == 0) {
        t1 = (line - p->verts[e1a]->pass[pass].u) / (p->verts[e1b]->pass[pass].u - p->verts[e1a]->pass[pass].u);
        t2 = (line - p->verts[e2a]->pass[pass].u) / (p->verts[e2b]->pass[pass].u - p->verts[e2a]->pass[pass].u);
    } else {
        t1 = (line - p->verts[e1a]->pass[pass].v) / (p->verts[e1b]->pass[pass].v - p->verts[e1a]->pass[pass].v);
        t2 = (line - p->verts[e2a]->pass[pass].v) / (p->verts[e2b]->pass[pass].v - p->verts[e2a]->pass[pass].v);
    }
    n1 = gr_poly_split_edge_vertex(p->verts[e1a], p->verts[e1b], t1, npasses, passes);
    n2 = gr_poly_split_edge_vertex(p->verts[e2a], p->verts[e2b], t2, npasses, passes);
    if (axis == 0) {
        n1->pass[pass].u = line;
        n2->pass[pass].u = line;
    } else {
        n1->pass[pass].v = line;
        n2->pass[pass].v = line;
    }
    a->nv = 0;
    for (j = 0; j < p->nv; j++) {
        k = (e1b + j) % p->nv;
        if (k == e2b)
            break;
        a->verts[a->nv] = p->verts[k];
        a->nv++;
    }
    a->verts[a->nv] = n2;
    a->nv++;
    a->verts[a->nv] = n1;
    a->nv++;
    b->nv = 0;
    for (j = 0; j < p->nv; j++) {
        k = (e2b + j) % p->nv;
        if (k == e1b)
            break;
        b->verts[b->nv] = p->verts[k];
        b->nv++;
    }
    b->verts[b->nv] = n1;
    b->nv++;
    b->verts[b->nv] = n2;
    b->nv++;
    return true;
}

// 0x55f8b0  Confidence: exact (float_min / float_max out of line, D4004).
bool float_strictly_between(float a, float b, float x)
{
    float lo = float_min(a, b);
    float hi = float_max(a, b);
    if (x > lo && x < hi)
        return true;
    return false;
}

// 0x55f900  Confidence: likely (the colour bytes are converted to float before the lerps, as in
// gr_clip_edge).
gr_multipass_vertex *gr_poly_split_edge_vertex(gr_multipass_vertex *a, gr_multipass_vertex *b, float t,
                                               int npasses, gr_tmapper_pass *passes)
{
    gr_multipass_vertex *n = gr_poly_alloc_clip_vertex();
    gr_pass_vertex_data *pa;
    gr_pass_vertex_data *pb;
    gr_pass_vertex_data *pn;
    float r, g, bl, al;
    int i;

    *(vector3 *)n->v = (*(vector3 *)b->v - *(vector3 *)a->v) * t + *(vector3 *)a->v;
    n->v->codes = 0;
    n->v->flags = 0;
    gr_project_vertex(n->v);
    for (i = 0; i < npasses; i++) {
        pa = &a->pass[i];
        pb = &b->pass[i];
        pn = &n->pass[i];
        if (passes[i].enabled) {
            pn->u = (pb->u - pa->u) * t + pa->u;
            pn->v = (pb->v - pa->v) * t + pa->v;
        }
        if (passes[i].vertex_color) {
            r = pa->r;
            g = pa->g;
            bl = pa->b;
            pn->r = (ubyte)(int)((pb->r - r) * t + r);
            pn->g = (ubyte)(int)((pb->g - g) * t + g);
            pn->b = (ubyte)(int)((pb->b - bl) * t + bl);
        }
        if (passes[i].vertex_alpha) {
            al = pa->a;
            pn->a = (ubyte)(int)((pb->a - al) * t + al);
        }
    }
    return n;
}

// 0x55fb00  Confidence: exact. The section of the bitmap (in card-sized pieces) that holds the polygon's
// mean uv, and the uv offset and scale into it. The bitmap size is clamped in place (bw / bh hold the
// imax results; D16201). LOGIC FIX (D16201): the scales are bitmap size / card size (0x55fc1e and
// 0x55fc3c: fdiv st, st(1) with st = the bitmap size), not the inverse.
int gr_poly_get_section(int nv, gr_multipass_vertex **verts, int pass, int bm, float *u_offset,
                        float *v_offset, float *u_scale, float *v_scale)
{
    int bw, bh;
    int cols, rows;
    int col, row;
    float su, sv;
    float n;
    int i;

    bm_get_dimensions(bm, &bw, &bh);
    bw = imax(bw, gr_screen.max_bitmap_w);
    bh = imax(bh, gr_screen.max_bitmap_h);
    su = 0.0f;
    sv = 0.0f;
    for (i = 0; i < nv; i++) {
        gr_multipass_vertex *v = verts[i];
        su += v->pass[pass].u;
        sv += v->pass[pass].v;
    }
    cols = (gr_screen.max_bitmap_w + bw - 1) / gr_screen.max_bitmap_w;
    n = (float)nv;
    rows = (gr_screen.max_bitmap_h + bh - 1) / gr_screen.max_bitmap_h;
    col = (int)floor(su / n * bw / gr_screen.max_bitmap_w);
    row = (int)floor(sv / n * bh / gr_screen.max_bitmap_h);
    *u_offset = (float)col * gr_screen.max_bitmap_w / bw;
    *u_scale = (float)bw / gr_screen.max_bitmap_w;
    *v_offset = (float)row * gr_screen.max_bitmap_h / bh;
    *v_scale = (float)bh / gr_screen.max_bitmap_h;
    return abs(row) % rows * cols + abs(col) % cols;
}
