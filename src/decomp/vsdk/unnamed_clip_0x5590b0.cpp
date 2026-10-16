// unnamed_clip_0x5590b0.cpp - Sum.exe v1.40, code 0x5590b0-0x559da7: the 3D polygon / line clipper with
// its temporary vertices, a view-sphere test and the bounding box helpers (vsdk\gr in sum.symbols). The
// file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): the static initialiser 0x5590b0 (table entry 648, the temporary vertex
// array) opens the object; bbox_transform (0x559c20) is the last regular function and vector3's
// operator[] (0x559da0) its COMDAT; lfsr_cipher (tagged vsdk\vfile) follows. .bss 0x2f53b00-0x2f54703
// (after the String file's 0x2f53aff), no .data; .rdata: the silhouette table 0x581f98 and the 0.7071
// constants 0x581f90 / 0x581f94.

#include "vsdk/gr/unnamed_clip_0x5590b0.h"

// ---- .bss -------------------------------------------------------------------------------------------
// 0x5590b0  init_gr_clip_temp_verts (jmp 0x5590c0). Confidence: likely (compiler-generated).
// 0x5590c0  the initialiser body: `vector constructor iterator` (0x404880) over the 64 vertices with
//           gr__Vertex__Vertex (0x5081c0). Confidence: likely (compiler-generated). The array has an
//           initialiser only because gr_vertex has a default constructor in the original (D1605, still
//           open: gr.h declares none, so this definition compiles without one).
gr_vertex gr_clip_temp_verts[GR_CLIP_MAX_TEMP_VERTS];   // 0x2f53b00
int gr_clip_num_temp_verts;                             // 0x2f54700

// .rdata 0x581f98: per eye region (x, y, z each below / inside / above the box: 9x + 3y + z, 0 = inside)
// up to six outline corners, 8 ends the list (stride 8 ints, the last two always 0).
static const int bbox_silhouette_table[27][8] = {
    { 8, 8, 8, 8, 8, 8, 0, 0 }, { 3, 0, 1, 2, 8, 8, 0, 0 }, { 4, 7, 6, 5, 8, 8, 0, 0 },
    { 2, 1, 5, 6, 8, 8, 0, 0 }, { 6, 2, 3, 0, 1, 5, 0, 0 }, { 1, 5, 4, 7, 6, 2, 0, 0 },
    { 7, 4, 0, 3, 8, 8, 0, 0 }, { 2, 3, 7, 4, 0, 1, 0, 0 }, { 3, 7, 6, 5, 4, 0, 0, 0 },
    { 7, 3, 2, 6, 8, 8, 0, 0 }, { 7, 3, 0, 1, 2, 6, 0, 0 }, { 4, 7, 3, 2, 6, 5, 0, 0 },
    { 3, 2, 1, 5, 6, 7, 0, 0 }, { 3, 0, 1, 5, 6, 7, 0, 0 }, { 3, 2, 1, 5, 4, 7, 0, 0 },
    { 6, 7, 4, 0, 3, 2, 0, 0 }, { 6, 7, 4, 0, 1, 2, 0, 0 }, { 4, 0, 3, 2, 6, 5, 0, 0 },
    { 0, 4, 5, 1, 8, 8, 0, 0 }, { 3, 0, 4, 5, 1, 2, 0, 0 }, { 0, 4, 7, 6, 5, 1, 0, 0 },
    { 4, 5, 6, 2, 1, 0, 0, 0 }, { 4, 5, 6, 2, 3, 0, 0, 0 }, { 0, 4, 7, 6, 2, 1, 0, 0 },
    { 1, 0, 3, 7, 4, 5, 0, 0 }, { 3, 7, 4, 5, 1, 2, 0, 0 }, { 0, 3, 7, 6, 5, 1, 0, 0 },
};

// 0x5590e0  Confidence: likely (the side planes put the crossing on z = +-x or +-y; the colour lerps
// convert the near vertex's bytes to float first).
gr_vertex *gr_clip_edge(int plane, gr_vertex *on, gr_vertex *off)
{
    gr_vertex *tmp;
    float t;
    float a, b;
    float r, g, bl, al;
    int negate;
    ubyte f;

    tmp = gr_clip_alloc_temp_vertex();
    if (plane & GR_CLIP_FAR) {
        a = off->z - on->z;
        if (a == 0.0f)
            t = 1.0f;
        else
            t = (gr_far_clip_z - on->z) / a;
        tmp->z = gr_far_clip_z;
        tmp->x = (off->x - on->x) * t + on->x;
        tmp->y = (off->y - on->y) * t + on->y;
    } else {
        if (plane & (GR_CLIP_LEFT | GR_CLIP_RIGHT)) {
            a = on->x;
            b = off->x;
        } else {
            a = on->y;
            b = off->y;
        }
        negate = plane & (GR_CLIP_LEFT | GR_CLIP_BOTTOM);
        if (negate) {
            a = -a;
            b = -b;
        }
        t = (a - on->z) / ((a - on->z) - b + off->z);
        tmp->x = (off->x - on->x) * t + on->x;
        tmp->y = (off->y - on->y) * t + on->y;
        if (plane & (GR_CLIP_BOTTOM | GR_CLIP_TOP))
            tmp->z = tmp->y;
        else
            tmp->z = tmp->x;
        if (negate)
            tmp->z = -tmp->z;
    }
    f = on->_unk1a & off->_unk1a;
    tmp->_unk1a = f;
    if (f & GR_VERTEX_UV) {
        tmp->u = (off->u - on->u) * t + on->u;
        tmp->v = (off->v - on->v) * t + on->v;
    }
    if (f & GR_VERTEX_UV2) {
        tmp->u2 = (off->u2 - on->u2) * t + on->u2;
        tmp->v2 = (off->v2 - on->v2) * t + on->v2;
    }
    if (f & GR_VERTEX_RGB) {
        r = on->r;
        g = on->g;
        bl = on->b;
        tmp->r = (ubyte)(int)((off->r - r) * t + r);
        tmp->g = (ubyte)(int)((off->g - g) * t + g);
        tmp->b = (ubyte)(int)((off->b - bl) * t + bl);
    }
    if (f & GR_VERTEX_ALPHA) {
        if (off->a == 0 && (plane & GR_CLIP_FAR)) {
            tmp->a = 0;
        } else {
            al = on->a;
            tmp->a = (ubyte)(int)((off->a - al) * t + al);
        }
    }
    gr_code_vertex(tmp);
    return tmp;
}

// 0x559330  Confidence: exact. No bound check against the 64 vertices.
gr_vertex *gr_clip_alloc_temp_vertex()
{
    gr_vertex *v = &gr_clip_temp_verts[gr_clip_num_temp_verts++];

    v->flags = 4;
    return v;
}

// 0x559350  Confidence: exact.
void gr_clip_line(gr_vertex **p0, gr_vertex **p1, ubyte codes)
{
    int bit;
    gr_vertex *tmp;

    for (bit = 1; bit < 0x20; bit <<= 1) {
        if (bit & codes) {
            if ((*p0)->codes & bit) {
                tmp = *p1;
                *p1 = *p0;
                *p0 = tmp;
            }
            tmp = gr_clip_edge(bit, *p0, *p1);
            *p1 = tmp;
            codes = (*p0)->codes | tmp->codes;
        }
    }
}

// 0x5593a0  Confidence: exact. src has room for n + 2 entries (the first two are repeated at the end).
int gr_clip_plane(int plane, gr_vertex **src, gr_vertex **dst, int *nv, ubyte *codes)
{
    gr_vertex **start = dst;
    gr_vertex *t;
    int i;

    src[*nv] = src[0];
    src[*nv + 1] = src[1];
    codes[1] = 0xff;
    codes[0] = 0;
    for (i = 1; i <= *nv; i++) {
        if (src[i]->codes & plane) {
            if (!(src[i - 1]->codes & plane)) {
                t = gr_clip_edge(plane, src[i - 1], src[i]);
                *dst++ = t;
                codes[0] |= t->codes;
                codes[1] &= t->codes;
            }
            if (!(src[i + 1]->codes & plane)) {
                t = gr_clip_edge(plane, src[i + 1], src[i]);
                *dst++ = t;
                codes[0] |= t->codes;
                codes[1] &= t->codes;
            }
        } else {
            *dst++ = src[i];
            codes[0] |= src[i]->codes;
            codes[1] &= src[i]->codes;
        }
    }
    return dst - start;
}

// 0x559490  Confidence: exact.
gr_vertex **gr_clip_polygon(gr_vertex **src, gr_vertex **tmp, int *nv, ubyte *codes)
{
    int bit;
    gr_vertex **t;

    for (bit = 1; bit < 0x20; bit <<= 1) {
        if (codes[0] & bit) {
            *nv = gr_clip_plane(bit, src, tmp, nv, codes);
            if (codes[1] != 0)
                return tmp;
            t = src;
            src = tmp;
            tmp = t;
        }
    }
    return src;
}

// 0x5594f0  Confidence: likely (the distances are kept in a local array for the second pass; z read once,
// D6803).
int gr_classify_sphere_view(const vector3 *p, float r)
{
    float d[4];
    float nz;
    float xs;
    float ys;

    float z = p->z;
    if (z + r <= 0.0f)
        return -1;
    nz = z * -0.70710677f;
    xs = p->x * 0.70710677f;
    d[0] = nz - xs;
    if (d[0] >= r)
        return -1;
    d[1] = xs + nz;
    if (d[1] >= r)
        return -1;
    ys = p->y * 0.70710677f;
    d[2] = ys + nz;
    if (d[2] >= r)
        return -1;
    d[3] = nz - ys;
    if (d[3] >= r)
        return -1;
    r = -r;
    if (d[0] < r && d[1] < r && d[2] < r && d[3] < r)
        return 1;
    return 0;
}

// 0x5595e0  Confidence: exact.
void bbox_from_points(vector3 *min, vector3 *max, const vector3 *a, const vector3 *b)
{
    if (a->x < b->x) {
        min->x = a->x;
        max->x = b->x;
    } else {
        min->x = b->x;
        max->x = a->x;
    }
    if (a->y < b->y) {
        min->y = a->y;
        max->y = b->y;
    } else {
        min->y = b->y;
        max->y = a->y;
    }
    if (a->z < b->z) {
        min->z = a->z;
        max->z = b->z;
    } else {
        min->z = b->z;
        max->z = a->z;
    }
}

// 0x559660  Confidence: exact.
void bbox_from_segment(vector3 *min, vector3 *max, const ix_ray *seg, float len)
{
    vector3 end = seg->origin + seg->dir * len;

    bbox_from_points(min, max, &seg->origin, &end);
}

// 0x5596b0  Confidence: exact.
bool bbox_overlap(const vector3 *amin, const vector3 *amax, const vector3 *bmin, const vector3 *bmax)
{
    if (amin->x <= bmax->x && amin->y <= bmax->y && amin->z <= bmax->z && amax->x >= bmin->x &&
        amax->y >= bmin->y && amax->z >= bmin->z)
        return true;
    return false;
}

// 0x559710  Confidence: exact (no symbol and no caller; name ours).
void bbox_add_point(vector3 *min, vector3 *max, const vector3 *p)
{
    if (p->x < min->x)
        min->x = p->x;
    else if (p->x > max->x)
        max->x = p->x;
    if (p->y < min->y)
        min->y = p->y;
    else if (p->y > max->y)
        max->y = p->y;
    if (p->z < min->z)
        min->z = p->z;
    else if (p->z > max->z)
        max->z = p->z;
}

// 0x559790  Confidence: exact (float_min / float_max out of line, D4004).
void bbox_union(vector3 *min, vector3 *max, const vector3 *bmin, const vector3 *bmax)
{
    min->x = float_min(min->x, bmin->x);
    min->y = float_min(min->y, bmin->y);
    min->z = float_min(min->z, bmin->z);
    max->x = float_max(max->x, bmax->x);
    max->y = float_max(max->y, bmax->y);
    max->z = float_max(max->z, bmax->z);
}

// 0x559810  Confidence: exact.
void bbox_expand_by_vector(vector3 *min, vector3 *max, const vector3 *v)
{
    if (v->x < 0.0f)
        min->x += v->x;
    else
        max->x += v->x;
    if (v->y < 0.0f)
        min->y += v->y;
    else
        max->y += v->y;
    if (v->z < 0.0f)
        min->z += v->z;
    else
        max->z += v->z;
}

// 0x559890  Confidence: exact.
int bbox_silhouette_corners(vector3 *out, const vector3 *p, const vector3 *min, const vector3 *max)
{
    int code = 0;
    int n;
    int i;

    if (p->x < min->x)
        code = 9;
    else if (p->x > max->x)
        code = 18;
    if (p->y < min->y)
        code += 3;
    else if (p->y > max->y)
        code += 6;
    if (p->z < min->z)
        code += 1;
    else if (p->z > max->z)
        code += 2;
    if (code == 0)
        return 0;
    n = 0;
    for (i = 0; i < 6; i++) {
        if (bbox_silhouette_table[code][i] > 7)
            break;
        bbox_get_corner(out, bbox_silhouette_table[code][i], min, max);
        n++;
        out++;
    }
    return n;
}

// 0x559960  Confidence: exact (an eight-entry jump table at 0x559a54; vector3::set out of line).
void bbox_get_corner(vector3 *out, int corner, const vector3 *min, const vector3 *max)
{
    switch (corner) {
    case 0: out->set(max->x, max->y, min->z); break;
    case 1: out->set(max->x, min->y, min->z); break;
    case 2: out->set(min->x, min->y, min->z); break;
    case 3: out->set(min->x, max->y, min->z); break;
    case 4: out->set(max->x, max->y, max->z); break;
    case 5: out->set(max->x, min->y, max->z); break;
    case 6: out->set(min->x, min->y, max->z); break;
    case 7: out->set(min->x, max->y, max->z); break;
    }
}

// 0x559a80  Confidence: exact.
void bbox_corner_indices_from_normal(const vector3 *n, int *near_corner, int *far_corner)
{
    if (n->x > 0.0f) {
        if (n->y > 0.0f) {
            if (n->z > 0.0f) {
                *far_corner = 4;
                *near_corner = 2;
            } else {
                *far_corner = 0;
                *near_corner = 6;
            }
        } else {
            if (n->z > 0.0f) {
                *far_corner = 5;
                *near_corner = 3;
            } else {
                *far_corner = 1;
                *near_corner = 7;
            }
        }
    } else {
        if (n->y > 0.0f) {
            if (n->z > 0.0f) {
                *far_corner = 7;
                *near_corner = 1;
            } else {
                *far_corner = 3;
                *near_corner = 5;
            }
        } else {
            if (n->z > 0.0f) {
                *far_corner = 6;
                *near_corner = 0;
            } else {
                *far_corner = 2;
                *near_corner = 4;
            }
        }
    }
}

// 0x559b80  Confidence: exact. (min, max, plane): the plane is the third argument (D6802; no caller).
bool bbox_straddles_plane(const vector3 *min, const vector3 *max, plane *p)
{
    vector3 c;
    int near_corner;
    int far_corner;

    bbox_corner_indices_from_normal(&p->normal, &near_corner, &far_corner);
    bbox_get_corner(&c, near_corner, min, max);
    if (p->distance_to_point(&c) > 0.0f)
        return false;
    bbox_get_corner(&c, far_corner, min, max);
    if (p->distance_to_point(&c) < 0.0f)
        return false;
    return true;
}

// 0x559c20  Confidence: exact (no symbol and no caller; name ours; operand order of the products).
void bbox_transform(matrix *m, const vector3 *pos, vector3 *min, vector3 *max, vector3 *out_min,
                    vector3 *out_max)
{
    int i;
    float a;
    float b;

    *out_min = *pos;
    *out_max = *pos;
    for (i = 0; i < 3; i++) {
        a = m->rvec[i] * (*min)[i];
        b = m->rvec[i] * (*max)[i];
        if (a < b) {
            out_min->x += a;
            out_max->x += b;
        } else {
            out_min->x += b;
            out_max->x += a;
        }
    }
    for (i = 0; i < 3; i++) {
        a = m->uvec[i] * (*min)[i];
        b = m->uvec[i] * (*max)[i];
        if (a < b) {
            out_min->y += a;
            out_max->y += b;
        } else {
            out_min->y += b;
            out_max->y += a;
        }
    }
    for (i = 0; i < 3; i++) {
        a = m->fvec[i] * (*min)[i];
        b = m->fvec[i] * (*max)[i];
        if (a < b) {
            out_min->z += a;
            out_max->z += b;
        } else {
            out_min->z += b;
            out_max->z += a;
        }
    }
}

// 0x559da0 vector3::operator[] (COMDAT, inline in vector.h). Confidence: exact.
