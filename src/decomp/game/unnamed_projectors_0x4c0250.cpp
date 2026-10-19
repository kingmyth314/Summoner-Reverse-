// unnamed_projectors_0x4c0250.cpp - Sum.exe v1.40, code 0x4c0250-0x4c0a3e: projected records (the base of
// the decal and blob-shadow records), the level-object BSP walk that marks the faces they fall on, and
// the per-frame face mark bitmap. File name NOT known: a placeholder (src/decomp/README.md). No assert
// path, no strings, no strings.tbl block, no static initialiser.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: projector_collect_lobj_faces (0x4c0250) to the plane assignment COMDAT (0x4c0a20-0x4c0a3e).
// The AI combat unit ends at 0x4c0245 (its jump tables); the lamp glow unit opens with its initialiser at
// address 0x4c0a40 (entry 317). This unit has no initialiser, so it is cut by its .bss: lobj_face_marks
// 0x2483564 follows the AI combat unit's ai_combat_aim_point (0x2483558-0x2483563), and the single-face
// word 0x24893e4 ends it (lamp_glows starts at 0x24893e8). No .data.

#include <math.h>
#include <string.h>
#include "vsdk/os/profile.h"
#include "engine/rendering/unnamed_projectors_0x4c0250.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/math/ix.h"

#define BSP_NODE_STACK 128
#define BSP_UNPACK(v) ((float)(v) * (2.0f / 1023.0f) - 1.0f)   // 0x581ba4 (2/1023), 0x57e990 (1.0)

// ---- .bss (address order) -----------------------------------------------------------------------------
ubyte lobj_face_marks[0x2000];                              // 0x2483564
short lobj_face_mark_offsets[MAX_FACE_MARK_LOBJS];          // 0x2485564
// 0x24893e4: projector_collect_lobj_faces' single-face word (a function-local static below).

// 0x4c0250  Confidence: likely (the two node stacks are locals of 128 entries each, far children at
// esp+0x7c and nodes at esp+0x27c; the node's signed distance is recomputed for every face of a popped
// node, as the binary does). A node whose plane the record sphere straddles is pushed and its near child
// visited first; popping it processes its faces and then visits its far child. Every face of the popped
// node is tested against the node plane (sphere reaching behind it), then clipped against the record's
// object-space planes; a shadow record also collects the level object once (D3900: the receiver count is
// a 3-bit field, so the ">= 16" exit never fires and the ninth object wraps the list; NOTES, game bugs).
void projector_collect_lobj_faces(s3d_geom_block *bsp, geom_bsp_node *node, int lobj_index,
                                  projector_record *rec)
{
    static ushort single_face;                  // 0x24893e4
    geom_bsp_node *far_stack[BSP_NODE_STACK];
    geom_bsp_node *node_stack[BSP_NODE_STACK];
    vector3 tri[3];
    geom_bsp_node *nodes;
    lobj *lo;
    s3d_geometry *mesh;
    s3d_face *f;
    shadow_record *sh;
    ushort *list;
    unsigned int count;
    unsigned int i;
    unsigned int near_child;
    unsigned int far_child;
    unsigned int front;
    unsigned int back;
    int sp;
    int face;
    int n;
    float dist;

    if (node == NULL)
        return;
    lo = &lobjs[lobj_index];
    mesh = lo->mesh;
    nodes = (geom_bsp_node *)bsp->_unk04;
    sp = 0;
    for (;;) {
        dist = BSP_UNPACK(node->nx) * rec->local_pos.x + BSP_UNPACK(node->ny) * rec->local_pos.y
             + BSP_UNPACK(node->nz) * rec->local_pos.z + node->d;
        front = node->has_front ? node->child : 0;
        back = node->has_back ? (node->has_front ? node->child + 1 : node->child) : 0;
        if (dist < 0.0f) {
            near_child = back;
            far_child = front;
        } else {
            near_child = front;
            far_child = back;
        }
        if (rec->radius > (float)fabs(dist)) {
            if (sp >= BSP_NODE_STACK)
                goto pop;
            far_stack[sp] = far_child ? &nodes[far_child] : NULL;
            node_stack[sp] = node;
            sp++;
        }
        if (near_child) {
            node = &nodes[near_child];
            continue;
        }
pop:
        for (;;) {
            if (sp == 0)
                return;
            node = node_stack[sp - 1];
            sp--;
            if (!node->face_list) {
                single_face = (ushort)node->faces;
                count = 1;
                list = &single_face;
            } else {
                list = &bsp->_unk00[node->faces];
                count = *list++;
            }
            for (i = 0; i < count; i++, list++) {
                if (BSP_UNPACK(node->nx) * rec->local_pos.x + BSP_UNPACK(node->ny) * rec->local_pos.y
                    + BSP_UNPACK(node->nz) * rec->local_pos.z + node->d - rec->radius >= 0.0f)
                    continue;
                rec->used = 1;
                face = *list;
                f = &mesh->faces[face];
                tri[0] = mesh->vertices[f->v[0]];
                tri[1] = mesh->vertices[f->v[1]];
                tri[2] = mesh->vertices[f->v[2]];
                if (!polygon_touches_planes(3, tri, rec->local_planes, rec->num_planes, 0))
                    continue;
                if (rec->shadow) {
                    sh = (shadow_record *)rec;
                    n = sh->lobj_count;
                    if (n == 0 || sh->lobj_list[n - 1] != lo) {
                        sh->lobj_list[n] = lo;
                        sh->lobj_count++;
                        if (n >= 16)
                            return;
                    }
                }
                lobj_face_marks[lobj_face_mark_offsets[lobj_index] + face / 8] |= (ubyte)(1 << (face % 8));
            }
            node = far_stack[sp];
            if (node)
                break;
        }
    }
}

// 0x4c0670  Confidence: exact.
projector_record::projector_record()
{
    clear_planes();
    shadow = 0;
}

// 0x4c0690  Confidence: exact. The base constructor runs first.
shadow_record::shadow_record()
{
    shadow = 1;
}

// 0x4c06b0  Confidence: exact.
void projector_record::clear_planes()
{
    num_planes = 0;
}

// 0x4c06c0  Confidence: exact (clear_planes expanded; add_plane called).
void projector_record::set(const vector3 *pos, float radius, const plane &p)
{
    this->pos = *pos;
    this->radius = radius;
    num_planes = 0;
    add_plane(p);
}

// 0x4c06f0  Confidence: exact. A seventh plane is dropped silently.
void projector_record::add_plane(const plane &p)
{
    if (num_planes < PROJECTOR_MAX_PLANES) {
        planes[num_planes] = p;
        num_planes++;
    }
}

// 0x4c0740  Confidence: likely (the plane temporaries: the binary builds the local plane in one temporary,
// copies its normal once more through the vector3 copy constructor (operator='s returned copy) and moves it
// through a second temporary with the out-of-line plane::operator= 0x4c0a20). The position goes into the
// object's frame (rows of the orientation); each plane keeps its normal rotated, and d becomes
// dot(n * d + lo->pos, n) = d + dot(n, lo->pos) (the normals are unit length). The point goes through the
// vector3 operators (x and y rounded, z kept), the dot product summed x, y, then z (D9804).
void projector_record::to_lobj_space(lobj *lo)
{
    unsigned int i;
    vector3 rel(pos.x - lo->pos.x, pos.y - lo->pos.y, pos.z - lo->pos.z);
    plane lp;
    plane tmp;

    local_pos = rel.rotated(lo->orient);
    for (i = 0; i < num_planes; i++) {
        vector3 pt = planes[i].normal * planes[i].d;
        pt += lo->pos;
        lp.d = pt.x * planes[i].normal.x + pt.y * planes[i].normal.y + pt.z * planes[i].normal.z;
        lp.normal = planes[i].normal.rotated(lo->orient);
        tmp = lp;
        local_planes[i] = tmp;
    }
}

// 0x4c0870  Confidence: exact (one exit per outcome in the binary). Flag 0x10 stays set unless the level
// object's bounding sphere (centre +0x44, the geometry's radius +0x30) lies wholly in front of a plane.
void projector_record::test_bounds(lobj *lo)
{
    unsigned int i;

    for (i = 0; i < num_planes; i++) {
        if (planes[i].normal.dot(lo->centre) + planes[i].d > lo->mesh->_unk30) {
            inside = 0;
            return;
        }
    }
    inside = 1;
}

// 0x4c08d0  Confidence: exact. The exception frame (trylevel 0 for the whole body) is a profile_label scope
// (D6102, D7304; its text is ours); the loop counters are unsigned (jbe). Called per level object with the
// shadow and decal records that may touch it (levelrender_lobj_collect_projections).
void lobj_project_records(int lobj_index, projector_record **recs, unsigned int count)
{
    profile_label label("lobj_project_records");
    lobj *lo = &lobjs[lobj_index];
    unsigned int i;

    for (i = 0; i < count; i++) {
        recs[i]->to_lobj_space(lo);
        recs[i]->used = 1;
    }
    if (lo->mesh->block._unk04) {
        for (i = 0; i < count; i++) {
            recs[i]->used = 0;
            projector_collect_lobj_faces(&lo->mesh->block, (geom_bsp_node *)lo->mesh->block._unk04,
                                         lobj_index, recs[i]);
        }
    }
    for (i = 0; i < count; i++) {
        if (!recs[i]->used)
            recs[i]->inside = 0;
    }
}

// 0x4c09a0  Confidence: exact. Objects flagged 0x440 get (faces + 7) / 8 bytes each; the others -1. No
// bound check against the 0x2000-byte bitmap or the 8000 slots (levels stay well below).
void lobj_face_marks_init()
{
    int i;
    int offset = 0;

    for (i = 0; i < num_lobjs; i++) {
        if (lobjs[i].flags & 0x440) {
            int faces = lobjs[i].mesh->num_faces;
            lobj_face_mark_offsets[i] = (short)offset;
            offset += (faces + 7) / 8;
        } else {
            lobj_face_mark_offsets[i] = -1;
        }
    }
}

// 0x4c0a00  Confidence: exact (rep stosd of 0x800 dwords).
void lobj_face_marks_clear()
{
    memset(lobj_face_marks, 0, sizeof(lobj_face_marks));
}

// 0x4c0a20  Confidence: exact. plane::operator=, compiler generated (the implicit copy assignment, emitted
// out of line here for to_lobj_space; it returns this). No source text.
