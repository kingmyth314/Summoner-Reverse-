// engine/s3d/s3d.h - level geometry: the .s3d loader, its record types and the level object
// (Engine\s3d\s3d.cpp, path reference 0x46f201; code 0x46dd30-0x4705a3, decomp28). Not original source.
// Type names are ours unless sum.symbols has one; the field offsets are the ones the loader writes.
#ifndef ENGINE_S3D_S3D_H
#define ENGINE_S3D_S3D_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/stringpool.h"

class File;
struct lobj;
struct lobj_face;
struct door_info;

// One triangle of a geometry record (0x14 bytes, s3d_read_face). Flag 0x20: the texture of the face has
// alpha (s3d_mark_alpha_meshes).
struct s3d_face {
    ushort flags;                     // +0x00
    ushort v[3];                      // +0x02 vertex indices
    vector3 normal;                   // +0x08 recomputed from the vertices after loading
};

// Two corners per face of a lightmapped level object (lobj +0x10, s3d_read_box / s3d_read_mesh_ref).
struct s3d_box {
    vector3 mins;                     // +0x00
    vector3 maxs;                     // +0x0c
};

// Extra geometry data at s3d_geometry +0x38 (0x14 bytes). Unit 0x4bbe80-0x4bcdc0 (decals and these
// helpers; file not known; source game/unnamed_decals_0x4bbe80.cpp, decomp56). Layout from the sum.symbols comment of s3d_geom_block_read.
class s3d_geom_block {
public:
    s3d_geom_block();                 // 0x4bcd40 s3d_geom_block__ctor (s3d_geom_block_clear)
    ~s3d_geom_block();                // 0x4bcd90 (thunk to the destructor)
    void read(File *f);               // 0x4bcdc0 s3d_geom_block_read
    void clear();                     // 0x4bcda0 s3d_geom_block_clear (decomp56)
    ushort *_unk00;                   // +0x00
    void *_unk04;                     // +0x04 12-byte entries
    unsigned int _unk08;              // +0x08
    unsigned int _unk0c;              // +0x0c
    unsigned int _unk10;              // +0x10
};
LAYOUT_CHECK_32(s3d_geom_block, sizeof(s3d_geom_block) == 0x14);

// A geometry (mesh shape) record, 0x54 bytes, shared by the level objects (lobj +0x04). The vertex,
// normal, uv and int16 arrays and the faces are slices of the level pools (s3d_read_geometry). The
// constructor 0x46fdb0 and destructor 0x46e190 are the compiler's (they construct / destroy +0x38).
// decomp76 (D5801): the render groups of a geometry (4 bytes each, geometry +0x4c): the faces and the
// vertices are stored group after group, and each group is drawn with the texture of its first face
// (levelrender_draw_list_*; names ours).
struct s3d_render_group {
    short num_faces;                  // +0x00
    short num_vertices;               // +0x02
};
struct s3d_geometry {
    char *name;                       // +0x00
    int _unk04;                       // +0x04
    int num_vertices;                 // +0x08
    vector3 *vertices;                // +0x0c
    vector3 *normals;                 // +0x10
    vector2 *uvs;                     // +0x14
    union {                           // +0x18 one int16 per vertex
        short *_unk18;
        short *vertex_dup;            //       decomp76: >= 0 the vertex repeats the one that many places
                                      //       before it (same position, a uv seam), < 0 a vertex of its own
    };
    int num_faces;                    // +0x1c
    s3d_face *faces;                  // +0x20
    vector3 _unk24;                   // +0x24
    float _unk30;                     // +0x30 the bounding radius (lobj_raycast_draw_lists; the draw lists
                                      //       pick the fog range by it against Small_mesh_threshold)
    int _unk34;                       // +0x34
    s3d_geom_block block;             // +0x38
    union {                           // +0x4c _unk50 ints (level mempool)
        int *_unk4c;
        s3d_render_group *groups;     //       decomp76
    };
    union {                           // +0x50
        int _unk50;
        int num_groups;               //       decomp76
    };
};
LAYOUT_CHECK_32(s3d_geometry, sizeof(s3d_geometry) == 0x54);

// Level +0x30 records (0x10 bytes; s3d_read_vertex_set).
struct s3d_vertex_set {
    int count;                        // +0x00
    int _unk04;                       // +0x04
    ubyte _unk08[4];                  // +0x08
    vector3 *verts;                   // +0x0c slice of the vertex pool
};

// A level navpoint (0x34 bytes, level +0x3c; s3d_read_navpoint). "$npc", "$hostile" and "$player" ones
// go to level_add_navpoint.
struct s3d_navpoint {
    char *name;                       // +0x00
    matrix orient;                    // +0x04 stored in the file as a quaternion
    vector3 pos;                      // +0x28
};
LAYOUT_CHECK_32(s3d_navpoint, sizeof(s3d_navpoint) == 0x34);

// Level +0x40 records (0x10 bytes; s3d_read_named_path). Doors name themselves after one (+0x04 of
// the door record).
struct s3d_named_path {
    char *name;                       // +0x00
    int _unk04;                       // +0x04
    int count;                        // +0x08
    vector3 *points;                  // +0x0c slice of the vertex pool
};

// A level light (0x4c bytes, level +0x4c; s3d_read_light, level_lights_setup).
struct s3d_light {
    int type;                         // +0x00 0 point, 1 and 4 spot
    quaternion orient;                // +0x04
    vector3 pos;                      // +0x14
    float r, g, b;                    // +0x20
    float intensity;                  // +0x2c (times level_light_scale)
    float _unk30;                     // +0x30 spot arguments
    float _unk34;                     // +0x34
    float range_a;                    // +0x38 used when flags bit 1 is set, else 20000
    float range_b;                    // +0x3c
    int flags;                        // +0x40
    float _unk44;                     // +0x44
    int texture;                      // +0x48 index into the level textures, -1 none
};
LAYOUT_CHECK(s3d_light, sizeof(s3d_light) == 0x4c);

// Level +0x50 records (0xc bytes; s3d_read_vis_set). "$vis..." sets mark their objects with lobj
// flag 0x4000 while level_load_s3d builds the $vis_base list.
struct s3d_vis_set {
    char *name;                       // +0x00
    ushort count;                     // +0x04
    int *indices;                     // +0x08 level object indices (int pool)
};

// Level +0x58 records (0x10 bytes; s3d_read_color_record).
struct s3d_color_record {
    float r, g, b;                    // +0x00 clamped after reading (see the note in s3d.cpp)
    int _unk0c;                       // +0x0c
};

// Level +0x68: one {name, door index} pair per door, sorted by name for door_find_by_name.
struct s3d_door_name {
    const char *name;                 // +0x00 the named path's name, "" without one
    int door;                         // +0x04
};

// Level +0x70 records (0x20 bytes; s3d_read_textured_record): the waving level objects (lobj flag
// 0x20, lobj_find_textured_record, levelrender_wave_lobj).
struct s3d_textured_record {
    int lobj;                         // +0x00 level object index (bsearch key)
    float scale_x;                    // +0x04 the wave divides the rotated vertex x by it
    float scale_z;                    // +0x08 and z by this
    float amplitude;                  // +0x0c
    float period;                     // +0x10 seconds
    int bm;                           // +0x14 bitmap, -1 none
    unsigned int num_weights;         // +0x18
    float *weights;                   // +0x1c one per vertex
};
LAYOUT_CHECK_32(s3d_textured_record, sizeof(s3d_textured_record) == 0x20);

// The level (0xe0 bytes at 0xa5a2a0, level_s3d). sum.symbols names several fields as globals
// (num_lobjs 0xa5a2c8, lobjs 0xa5a2cc, level_num_textures 0xa5a318, ...); they are the macros below
// (D800). The *_pool arrays are allocated once per level in s3d_alloc_level_arrays and handed out in
// slices by the readers (cursors s3d_*_cursor in s3d.cpp's .bss).
struct s3d_level {
    ubyte flags;                      // +0x00 bit 0 loaded, bit 1 textures paged in
    ubyte _pad01[3];
    int version;                      // +0x04
    StringPool names;                 // +0x08
    char *filename;                   // +0x1c
    int num_geometry;                 // +0x20
    s3d_geometry *geometry;           // +0x24
    int num_meshes;                   // +0x28 num_lobjs
    lobj *meshes;                     // +0x2c lobjs (0x70 bytes each)
    int num_vertex_sets;              // +0x30
    s3d_vertex_set *vertex_sets;      // +0x34
    int num_navpoints;                // +0x38 level_num_navpoints
    s3d_navpoint *navpoints;          // +0x3c level_navpoints
    int num_named_paths;              // +0x40
    s3d_named_path *named_paths;      // +0x44
    int num_lights;                   // +0x48
    s3d_light *lights;                // +0x4c
    int num_vis_sets;                 // +0x50 level_num_vis_records
    s3d_vis_set *vis_sets;            // +0x54 level_vis_records
    int num_color_records;            // +0x58
    s3d_color_record *color_records;  // +0x5c
    int num_door_records;             // +0x60 num_doors
    door_info *door_records;          // +0x64 doors (0x8c bytes each)
    s3d_door_name *door_names;        // +0x68
    int num_textured;                 // +0x6c
    s3d_textured_record *textured;    // +0x70
    int num_lightmaps;                // +0x74
    int num_textures;                 // +0x78 level_num_textures
    int *lightmap_bms;                // +0x7c level_face_bitmaps (-1 when not loaded)
    int *texture_bms;                 // +0x80 level_texture_bms
    int texture_names_size;           // +0x84
    char *texture_names;              // +0x88 NUL-separated names
    int _unk8c;                       // +0x8c
    int num_pool_faces;               // +0x90
    int num_pool_mesh_faces;          // +0x94
    int num_pool_vertices;            // +0x98
    int num_pool_normals;             // +0x9c (also the uv and int16 pools)
    int num_pool_ints;                // +0xa0
    int num_pool_bytes;               // +0xa4
    int _unka8;                       // +0xa8
    s3d_face *face_pool;              // +0xac
    lobj_face *mesh_face_pool;        // +0xb0
    vector3 *vertex_pool;             // +0xb4
    vector2 *uv_pool;                 // +0xb8
    int *int_pool;                    // +0xbc
    vector3 *normal_pool;             // +0xc0
    int _unkc4;                       // +0xc4
    short *short_pool;                // +0xc8
    ubyte *byte_pool;                 // +0xcc per-vertex RGB of the level objects
    int _unkd0;                       // +0xd0
    vector3 ambient;                  // +0xd4 (level_lights_setup)
};
LAYOUT_CHECK_32(s3d_level, sizeof(s3d_level) == 0xe0);

extern s3d_level level_s3d;           // 0xa5a2a0
#define num_lobjs (level_s3d.num_meshes)                // 0xa5a2c8
#define lobjs (level_s3d.meshes)                        // 0xa5a2cc
#define level_num_navpoints (level_s3d.num_navpoints)   // 0xa5a2d8
#define level_navpoints (level_s3d.navpoints)           // 0xa5a2dc
#define level_num_vis_records (level_s3d.num_vis_sets)  // 0xa5a2f0
#define level_vis_records (level_s3d.vis_sets)          // 0xa5a2f4
#define num_doors (level_s3d.num_door_records)          // 0xa5a300
#define doors (level_s3d.door_records)                  // 0xa5a304
#define level_num_textures (level_s3d.num_textures)     // 0xa5a318
#define level_face_bitmaps (level_s3d.lightmap_bms)     // 0xa5a31c
#define level_texture_bms (level_s3d.texture_bms)       // 0xa5a320

// Console variables (s3d.cpp .data / .bss).
extern bool S3d_load_lightmaps;       // 0x59aa58 console "load_lightmaps"; GameInit clears it without multitexture
extern bool Test_meshes_on_load;      // 0x22e4423
extern bool P3D_files;                // 0x22e4424
extern bool s3d_version_flag;         // 0x22e4425 set while loading "ionaext.s3d" (Ocean meshes)
extern bool s3d_lightmaps_loaded;     // 0x22e4420 (name ours)

// The $vis_base set: every level object that no "$vis" set names.
extern s3d_vis_set level_vis_base;       // 0x22e43f8 the $vis_base set (name +0, count +4, level objects +8; D9813)
extern bool level_vis_base_built;         // 0x22e4422

int  level_load_s3d(const char *filename, s3d_level *level, int names_size);  // 0x46e1b0 -1 on failure
void level_free_s3d(s3d_level *level);    // 0x46fdc0
void level_page_in_textures();            // 0x46fef0

#endif
