// unnamed_decals_0x4bbe80.cpp - Sum.exe v1.40, code 0x4bbe80-0x4bce55: level decals and the s3d_geom_block
// members. File name NOT known: a placeholder (src/decomp/README.md). No assert path, no strings.tbl block;
// cut by the static initialiser run (entries 314-315, 0x5884ec-0x5884f0) and the data.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_decal_records (0x4bbe80) to s3d_geom_block_read (0x4bcdc0-0x4bce55). The next
// initialiser, 0x4bce60 (entry 316), opens the AI combat unit.
// Data: .data 0x59daf0-0x59db6f (decal_vert_ptrs, 32 pointers into decal_verts); .bss 0x2480948-0x2483557
// (decal_nodes first, decal_age last). Floats are pooled .rdata constants.
// The vector code here calls the vector3 constructor, copy constructor, operator=, +, - and the scale
// function out of line (0x43b580, 0x410cd0, 0x413190, 0x45b680, 0x45b6b0, 0x468a90), unlike most optimised
// units; vector3::len is called once (the COMDAT 0x4bc290 sits in this unit) and expanded once.
// decals_draw_for_lobj has an exception frame (a local with a destructor that the code does not show).
// The rings are vsdk.h's plist_* lists (head pointer, NULL when empty); decal_recycle's body is expanded
// again in decals_draw_for_lobj and decals_recycle_unused.

#include <math.h>
#include "engine/rendering/unnamed_decals_0x4bbe80.h"
#include "engine/rendering/unnamed_projectors_0x4c0250.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "vsdk/gr/gr.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/ix.h"
#include "vsdk/vfile/file.h"

// ---- .bss ---------------------------------------------------------------------------------------------
decal_node decal_nodes[MAX_DECALS];   // 0x2480948
decal_record decal_records[MAX_DECALS];   // 0x2480a48 (init_decal_records)
gr_vertex decal_verts[MAX_DECALS];    // 0x2482f48
int decal_2483548;                    // 0x2483548
decal_record *decal_free;             // 0x248354c
decal_record *decal_active;           // 0x2483550
decal_node *decal_age;                // 0x2483554

// ---- .data --------------------------------------------------------------------------------------------
gr_vertex *decal_vert_ptrs[MAX_DECALS] = {   // 0x59daf0
    &decal_verts[0],  &decal_verts[1],  &decal_verts[2],  &decal_verts[3],
    &decal_verts[4],  &decal_verts[5],  &decal_verts[6],  &decal_verts[7],
    &decal_verts[8],  &decal_verts[9],  &decal_verts[10], &decal_verts[11],
    &decal_verts[12], &decal_verts[13], &decal_verts[14], &decal_verts[15],
    &decal_verts[16], &decal_verts[17], &decal_verts[18], &decal_verts[19],
    &decal_verts[20], &decal_verts[21], &decal_verts[22], &decal_verts[23],
    &decal_verts[24], &decal_verts[25], &decal_verts[26], &decal_verts[27],
    &decal_verts[28], &decal_verts[29], &decal_verts[30], &decal_verts[31],
};

// 0x4bbe80  Confidence: exact. static_init_decal_records: "jmp 0x4bbe90" (initialiser entry 314).
// 0x4bbe90  Confidence: exact. init_decal_records: decal_record() for the 32 records (stride 0x128), the
// compiler's initialiser for decal_records; no destructor, so no atexit.
// 0x4bbeb0  Confidence: exact. static_init_empty_4bbec0: "jmp 0x4bbec0" (entry 315).
// 0x4bbec0  Confidence: likely. init_empty_4bbec0: a bare ret. The initialiser of an object without
// construction code that the compiler still registers; which definition (decal_verts: gr_vertex has no
// constructor here, D1605) is not known.

// 0x4bbed0  Confidence: exact. The members' constructors run first (GameTimer at +0, the projector at +0x38).
decal_record::decal_record()
{
    timer.invalidate();
    bitmap = -1;
    next = prev = NULL;
    clipped = 0;
    in_use = 0;
}

// 0x4bbf10  Confidence: unsure (the vector temporaries; the operators are called out of line here). The
// decal frame: normal u x v; depth = normal * sqrt(larger of |u|^2, |v|^2) * depth; six planes: +u (the
// frame plane, v coordinate), -u, +v (u coordinate), -v, +depth, -depth.
void decal_build_clip_planes(decal_record *d, const vector3 *pos, const vector3 *u, const vector3 *v)
{
    vector3 depth;
    float lu;
    float lv;

    d->pos = *pos;
    vector3 un = u->normalized();
    vector3 vn = v->normalized();
    d->normal = vector3(v->z * u->y - v->y * u->z, u->z * v->x - v->z * u->x, v->y * u->x - u->y * v->x);
    d->normal.normalize();
    lu = u->len_sq();
    lv = v->len_sq();
    if (lu > lv)
        depth = vector3(d->normal.x * ((float)sqrt(lu) * d->depth), d->normal.y * ((float)sqrt(lu) * d->depth),
                        d->normal.z * ((float)sqrt(lu) * d->depth));
    else
        depth = vector3(d->normal.x * ((float)sqrt(lv) * d->depth), d->normal.y * ((float)sqrt(lv) * d->depth),
                        d->normal.z * ((float)sqrt(lv) * d->depth));
    vector3 diag(u->x + v->x, u->y + v->y, u->z + v->z);
    vector3 corner(diag.x + depth.x, diag.y + depth.y, diag.z + depth.z);
    d->radius = (float)sqrt(corner.x * corner.x + corner.y * corner.y + corner.z * corner.z);

    vector3 p(d->pos.x + u->x, d->pos.y + u->y, d->pos.z + u->z);
    d->set_frame(plane(&un, &p));
    d->set_v_axis(0.5f / u->len());
    p = d->pos - *u;
    vector3 n = -1.0f * un;
    d->proj.add_plane(plane(&n, &p));
    p = d->pos + *v;
    d->proj.add_plane(plane(&vn, &p));
    d->set_u_axis(0.5f / v->len());
    p = d->pos - *v;
    n = -1.0f * vn;
    d->proj.add_plane(plane(&n, &p));
    p = d->pos + depth;
    d->proj.add_plane(plane(&d->normal, &p));
    p = d->pos - depth;
    n = -1.0f * d->normal;
    d->proj.add_plane(plane(&n, &p));
}

// 0x4bc290  Confidence: exact. vector3__len: the COMDAT of the inline vector3::len (vsdk/math/vector.h),
// emitted here for the out-of-line calls above; not written again.

// 0x4bc2c0  Confidence: exact.
void decal_record::set_frame(const plane &p)
{
    proj.set(&pos, radius, p);
}

// 0x4bc2e0  Confidence: exact.
void decal_record::set_u_axis(float scale)
{
    u_scale = scale;
    u_plane = proj.num_planes - 1;
}

// 0x4bc300  Confidence: exact.
void decal_record::set_v_axis(float scale)
{
    v_scale = scale;
    v_plane = proj.num_planes - 1;
}

// 0x4bc320  Confidence: likely (the age-ring insertion loop). Takes a free record (with none free, the
// oldest one by the age ring), clips it, puts it on the active ring and its node into the age ring before
// the first decal with more time left (unlimited ones count as 0x7fffffff). Returns the record index.
int decal_place(int bitmap, const vector3 *pos, const vector3 *u, const vector3 *v, int ms, bool lit,
                int colour, float depth)
{
    decal_record *d;
    decal_record *other;
    decal_node *node;
    decal_node *n;
    int index;
    int left;

    d = decal_free;
    if (d == NULL && decal_age != NULL) {
        decal_recycle(&decal_records[decal_age - decal_nodes], decal_age);
        d = decal_free;
    }
    plist_remove(decal_free, d);
    index = d - decal_records;
    node = &decal_nodes[index];
    d->timer.SetTimeout(0);
    d->bitmap = bitmap;
    d->lifetime = ms;
    d->lit = lit;
    d->in_use = 1;
    d->colour = colour;
    d->depth = depth;
    decal_build_clip_planes(d, pos, u, v);
    d->clipped = 1;
    plist_append(decal_active, d);
    if (decal_age == NULL) {
        node->next = node;
        node->prev = node;
        decal_age = node;
        return index;
    }
    n = decal_age;
    do {
        other = &decal_records[n - decal_nodes];
        if (other->lifetime)
            left = other->lifetime - other->timer.GetElapsedTime();
        else
            left = 0x7fffffff;
        if (left > (int)(d->lifetime - d->timer.GetElapsedTime())) {
            n->prev->next = node;
            node->prev = n->prev;
            n->prev = node;
            node->next = n;
            if (decal_age == n)
                decal_age = node;
            return index;
        }
        n = n->next;
    } while (n != decal_age && n != NULL);
    plist_append(decal_age, node);
    return index;
}

// 0x4bc560  Confidence: exact. Off the active and age rings, onto the free ring.
void decal_recycle(decal_record *d, decal_node *n)
{
    plist_remove(decal_active, d);
    plist_remove(decal_age, n);
    plist_append(decal_free, d);
    d->in_use = 0;
}

// 0x4bc640  Confidence: exact.
void decals_reset()
{
    decal_record *d;

    decal_active = NULL;
    decal_free = NULL;
    decal_age = NULL;
    for (d = decal_records; d < &decal_records[MAX_DECALS]; d++) {
        plist_append(decal_free, d);
        d->in_use = 0;
    }
}

// 0x4bc6b0  Confidence: unsure (the exception frame's local is not known; the loops' shape is). For every
// active decal (expired ones are recycled first): when it touches the level object and is clipped, each
// marked face of the object (lobj_face_marks) that the decal's planes touch is clipped to them, gets u / v
// from the two axis planes (scaled), white or light-shaded colours, a fade-out over the last second of
// the lifetime, and is drawn with a small depth bias.
void decals_draw_for_lobj(int lobj_index, const vector3 *verts)
{
    lobj *lo = &lobjs[lobj_index];
    decal_record *d;
    decal_record *next;
    s3d_geometry *g;
    s3d_face *f;
    ubyte *marks;
    unsigned int bits;
    int num_bytes;
    int b;
    int k;
    int i;
    int n;
    unsigned int elapsed;
    float fade;
    ubyte alpha;
    ubyte vert_flags;
    plane *pl;
    vector3 tri[3];
    vector3 clipped[MAX_DECALS];

    d = decal_active;
    if (d == NULL)
        return;
    do {
        next = d->next != decal_active ? d->next : NULL;
        if (d->timer.GetElapsedTime() > (int)d->lifetime) {
            decal_node *node = &decal_nodes[d - decal_records];
            plist_remove(decal_active, d);
            plist_remove(decal_age, node);
            plist_append(decal_free, d);
            d->in_use = 0;
        }
        if (d->proj.inside && d->clipped) {
            marks = &lobj_face_marks[lobj_face_mark_offsets[lobj_index]];
            g = lo->mesh;
            num_bytes = (g->num_faces + 7) / 8;
            for (b = 0; b < num_bytes; b++) {
                bits = *marks++;
                if (bits == 0)
                    continue;
                for (k = 0; k < 8; k++, bits >>= 1) {
                    if (!(bits & 1))
                        continue;
                    f = &g->faces[b * 8 + k];
                    tri[0] = verts[f->v[0]];
                    tri[1] = verts[f->v[1]];
                    tri[2] = verts[f->v[2]];
                    if (!polygon_touches_planes(3, tri, d->proj.local_planes, d->proj.num_planes, 0))
                        continue;
                    n = polygon_clip_to_planes(3, tri, clipped, d->proj.local_planes, d->proj.num_planes);
                    if (n < 3)
                        continue;
                    elapsed = d->timer.GetElapsedTime();
                    if (d->lit)
                        lights_select_nearest(-1, &d->pos, d->radius);
                    if (d->lifetime && elapsed > d->lifetime - 1000) {
                        fade = (float)(elapsed - d->lifetime + 1000) * 0.001f;
                        if (fade > 1.0f)
                            fade = 1.0f;
                        vert_flags = 0xd;
                        alpha = (ubyte)(int)((1.0f - fade) * 255.0f);
                        for (i = 0; i < n; i++)
                            decal_verts[i].a = alpha;
                    } else {
                        vert_flags = 5;
                    }
                    for (i = 0; i < n; i++) {
                        decal_verts[i]._unk1a = vert_flags;
                        pl = &d->proj.local_planes[d->u_plane];
                        decal_verts[i].u = -((pl->normal.z * clipped[i].z + pl->normal.y * clipped[i].y
                                              + clipped[i].x * pl->normal.x + pl->d) * d->u_scale);
                        pl = &d->proj.local_planes[d->v_plane];
                        decal_verts[i].v = -((pl->normal.z * clipped[i].z + pl->normal.y * clipped[i].y
                                              + pl->normal.x * clipped[i].x + pl->d) * d->v_scale);
                        if (d->lit) {
                            lights_shade_vertices(&decal_verts[i].r, &decal_verts[i].g, &decal_verts[i].b,
                                                  &clipped[i], &d->normal, 1, 0, d->colour);
                        } else {
                            decal_verts[i].r = 0xff;
                            decal_verts[i].g = 0xff;
                            decal_verts[i].b = 0xff;
                        }
                    }
                    gr_rotate_vertices(decal_verts, clipped, n);
                    for (i = 0; i < n; i++)
                        gr_offset_point_depth(&decal_verts[i], -0.25f);
                    gr_set_texture(d->bitmap, -1);
                    gr_poly(n, decal_vert_ptrs, 0x118c21, 0, 0);
                    gr_poly_buffer_flush();
                    if (d->lit)
                        lights_pop_selection();
                }
            }
        }
        d = next;
    } while (d);
}

// 0x4bcbb0  Confidence: likely. Every touching decal counts, the list takes the first 0x40.
int decals_collect_for_lobj(lobj *lo, projector_record **out, int *count)
{
    decal_record *d = decal_active;
    int found = 0;

    if (d == NULL)
        return 0;
    do {
        d->proj.test_bounds(lo);
        if (d->proj.inside) {
            if (*count < 0x40) {
                out[*count] = &d->proj;
                (*count)++;
            }
            found++;
        }
        d = d->next;
    } while (d != decal_active && d != NULL);
    return found;
}

// 0x4bcc20  Confidence: likely. The one-frame decals (lifetime 0) go back to the free ring after the draw.
void decals_recycle_unused()
{
    decal_record *d = decal_active;
    decal_record *next;

    while (d) {
        next = d->next != decal_active ? d->next : NULL;
        if (d->lifetime == 0) {
            decal_node *node = &decal_nodes[d - decal_records];
            plist_remove(decal_active, d);
            plist_remove(decal_age, node);
            plist_append(decal_free, d);
            d->in_use = 0;
        }
        d = next;
    }
}

// 0x4bcd40  Confidence: exact.
s3d_geom_block::s3d_geom_block()
{
    clear();
}

// 0x4bcd50  Confidence: likely (delete[] calls 0x512570 like the other units' array deletes, D200).
// 0x4bcd90  Confidence: unsure (its origin). s3d_geom_block__dtor_thunk: "jmp 0x4bcd50", the address the
// eh vector iterator of s3d_alloc_level_arrays receives; no source of its own.
s3d_geom_block::~s3d_geom_block()
{
    if (_unk00) {
        delete[] _unk00;
        _unk00 = NULL;
    }
    if (_unk04) {
        delete[] (ubyte *)_unk04;
        _unk04 = NULL;
    }
}

// 0x4bcda0  Confidence: exact (store order 4, 8, 0xc, 0, 0x10).
void s3d_geom_block::clear()
{
    _unk04 = NULL;
    _unk08 = 0;
    _unk0c = 0;
    _unk00 = NULL;
    _unk10 = 0;
}

// 0x4bcdc0  Confidence: likely. Reads once (+0x04 still NULL): three counts, then _unk08 12-byte entries
// and _unk10 ushorts.
void s3d_geom_block::read(File *f)
{
    if (_unk04 != NULL)
        return;
    _unk08 = f->read_uint(0, 0);
    _unk0c = f->read_uint(0, 0);
    _unk10 = f->read_uint(0, 0);
    if (_unk08) {
        _unk04 = new ubyte[_unk08 * 12];
        f->read(_unk04, _unk08 * 12, 0, 0);
    }
    if (_unk10) {
        _unk00 = new ushort[_unk10];
        f->read(_unk00, _unk10 * 2, 0, 0);
    }
}
