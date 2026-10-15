// engine/rendering/unnamed_projectors_0x4c0250.h - projected records (the decal and blob-shadow base) and
// the per-frame level face marks: the unit 0x4c0250-0x4c0a3e (docs/sum-exe/source-map.md hand cuts; .bss
// 0x2483564-0x24893e7; source game/unnamed_projectors_0x4c0250.cpp, decomp59). File and directory names
// are placeholders: no path string, no strings.tbl block. Declared in decomp56 for the decal unit
// (game/unnamed_decals_0x4bbe80.cpp). Not original source; layouts from the record functions
// 0x4c0670-0x4c0870 and projector_collect_lobj_faces.
#ifndef ENGINE_RENDERING_UNNAMED_PROJECTORS_0X4C0250_H
#define ENGINE_RENDERING_UNNAMED_PROJECTORS_0X4C0250_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/ix.h"
#include "vsdk/math/matrix.h"           // decomp57: shadow_frame

struct lobj;
class s3d_geom_block;

#define PROJECTOR_MAX_PLANES 6

// A projected volume (0xe0 bytes): the decal records (+0x38) and the shadow records embed one. The flags
// word: bit 0 set per record by lobj_project_records (cleared again before the BSP walk, set when a face of
// the object is near: decomp59), bits 1-3 the plane count, bit 4 "touches the level object" (test_bounds),
// bit 5 shadow record. Type and field names ours.
class projector_record {
public:
    projector_record();                       // 0x4c0670 projector_record__ctor (clear_planes, flag 0x20 off)
    void clear_planes();                      // 0x4c06b0
    void set(const vector3 *pos, float radius, const plane &p);   // 0x4c06c0 (pos, radius, the first plane)
    void add_plane(const plane &p);           // 0x4c06f0 (at most 6)
    void to_lobj_space(lobj *lo);             // 0x4c0740 pos and planes into the object's frame
    void test_bounds(lobj *lo);               // 0x4c0870 flag 0x10 when the object's sphere is inside all planes

    unsigned int used : 1;                    // +0x00 bit 0
    unsigned int num_planes : 3;              //       bits 1-3
    unsigned int inside : 1;                  //       bit 4
    unsigned int shadow : 1;                  //       bit 5
    unsigned int lobj_count : 3;              //       bits 6-8 (decomp57): level objects in shadow_record::lobj_list
                                              //       (shadow_list_clear clears them; num_lobjs is a macro of s3d.h)
    unsigned int _flags_hi : 23;
    float radius;                             // +0x04
    vector3 pos;                              // +0x08
    plane planes[PROJECTOR_MAX_PLANES];       // +0x14 world space
    vector3 local_pos;                        // +0x74 to_lobj_space
    plane local_planes[PROJECTOR_MAX_PLANES]; // +0x80 the object's frame (the clip tests use these)
};
LAYOUT_CHECK(projector_record, sizeof(projector_record) == 0xe0);

// One bit per face of each level object with flags 0x440, set by projector_collect_lobj_faces and cleared
// every frame (lobj_face_marks_clear); the per-object start offsets (bytes, -1 none) from
// lobj_face_marks_init. sum.symbols types the offsets int16_t; one per level object.
// decomp57: the shadow record (the blob shadows of game/unnamed_shadows_0x4a37d0.cpp embed one at +0x74).
// The optional record shadow_record_add copies into it (0x64 bytes; the copy calls matrix::operator= and
// vector3::operator= for its members, three floats are copied as they are). Names ours.
struct shadow_frame {
    matrix m0;                                // +0x00
    vector3 v24;                              // +0x24
    float f30, f34, f38;                      // +0x30
    matrix m3c;                               // +0x3c
    int i60;                                  // +0x60
};
LAYOUT_CHECK(shadow_frame, sizeof(shadow_frame) == 0x64);
class shadow_record : public projector_record {
public:
    shadow_record();                          // 0x4c0690 shadow_record__ctor (projector_record(), then shadow = 1)
    lobj *lobj_list[7];                       // +0xe0 the level objects it touches (shadows_render). decomp59
                                              //       (D3900): projector_collect_lobj_faces appends with the 3-bit
                                              //       lobj_count, so an eighth object lands at +0xfc and a ninth wraps
    ubyte _unkfc[0x24];                       // +0xfc
    shadow_frame frame;                       // +0x120
};
LAYOUT_CHECK(shadow_record, sizeof(shadow_record) == 0x184);

// One node of a level geometry's BSP (12 bytes; s3d_geom_block +0x04, the root is the first). The normal
// is packed into three 10-bit fields (n = v * 2 / 1023 - 1); the front child (dist >= 0) is child, the back
// child child + has_front; a node holds one face (faces) or a list (face_list: faces indexes the ushort
// array at s3d_geom_block +0x00, whose first entry is the count). Type and field names ours (decomp59).
struct geom_bsp_node {
    unsigned int nx : 10;                     // +0x00
    unsigned int ny : 10;
    unsigned int nz : 10;
    unsigned int has_back : 1;                //       bit 30
    unsigned int has_front : 1;               //       bit 31
    float d;                                  // +0x04
    unsigned int child : 16;                  // +0x08
    unsigned int faces : 15;
    unsigned int face_list : 1;
};
LAYOUT_CHECK(geom_bsp_node, sizeof(geom_bsp_node) == 0xc);

// decomp59: 8000 offset slots, up to the walk's single-face word 0x24893e4 (lamp_glows follows at 0x24893e8).
#define MAX_FACE_MARK_LOBJS 8000
extern ubyte lobj_face_marks[0x2000];         // 0x2483564
extern short lobj_face_mark_offsets[MAX_FACE_MARK_LOBJS];   // 0x2485564

// decomp59: the unit's functions. (bsp, root, lobj index, record): the faces of the level object near the
// record's sphere and inside its planes get their mark bits; shadow records also collect the object.
void projector_collect_lobj_faces(s3d_geom_block *bsp, geom_bsp_node *node, int lobj_index,
                                  projector_record *rec);   // 0x4c0250
// (lobj index, records, count): records into the object's frame, then the face walk and the bounds flag.
// Caller levelrender_lobj_collect_projections (0x486520).
void lobj_project_records(int lobj_index, projector_record **recs, unsigned int count);   // 0x4c08d0
void lobj_face_marks_init();                  // 0x4c09a0
void lobj_face_marks_clear();                 // 0x4c0a00

#endif
