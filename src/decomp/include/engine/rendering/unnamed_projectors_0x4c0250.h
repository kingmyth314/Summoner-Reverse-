// engine/rendering/unnamed_projectors_0x4c0250.h - projected records (the decal and blob-shadow base) and
// the per-frame level face marks: the unit 0x4c0250-0x4c0a20 (docs/sum-exe/source-map.md hand cuts; .bss
// 0x2483564-0x24893e4). File and directory names are placeholders: no path string, no strings.tbl block.
// Not written yet; declared in decomp56 for the decal unit (game/unnamed_decals_0x4bbe80.cpp). Not
// original source; layouts from the record functions 0x4c0670-0x4c0870.
#ifndef ENGINE_RENDERING_UNNAMED_PROJECTORS_0X4C0250_H
#define ENGINE_RENDERING_UNNAMED_PROJECTORS_0X4C0250_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/ix.h"

struct lobj;

#define PROJECTOR_MAX_PLANES 6

// A projected volume (0xe0 bytes): the decal records (+0x38) and the shadow records embed one. The flags
// word: bit 0 set per record by lobj_project_records, bits 1-3 the plane count, bit 4 "touches the level
// object" (test_bounds), bit 5 shadow record. Type and field names ours.
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
    unsigned int _flags_hi : 26;
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
extern ubyte lobj_face_marks[0x2000];         // 0x2483564
extern short lobj_face_mark_offsets[];        // 0x2485564

#endif
