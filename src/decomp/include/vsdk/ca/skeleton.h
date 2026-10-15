// vsdk/ca/skeleton.h - animations (.mvf skeletons), key tracks and .vim meshes (vsdk\ca\skeleton.cpp,
// path references 0x543d8c-0x5449da; anim_track 0x550f90-0x551350 is in an unnamed unit). Not original
// source. Field names ours unless a symbol comment names them.
// decomp25: skeleton.cpp written (0x543040-0x544b7e); the layouts below are completed from it.
#ifndef VSDK_CA_SKELETON_H
#define VSDK_CA_SKELETON_H

#include "vsdk/math/matrix.h"

class FileReader;
class File;

// ---- key frames ---------------------------------------------------------------------------------------
// The key classes are small inline classes: their constructors are COMDATs after skeleton.cpp's last
// function (0x544ad0-0x544b7e, DECISIONS.md D502).
class anim_key {
public:
    anim_key(int time = 0) { this->time = time; }     // 0x544b20 anim_key__ctor
    int time;                         // +0x0 ticks
};

// Position key (0x28 bytes): position and the two curve control points. The constructor default-
// constructs the base (time 0) and ignores its time argument (0x544ad0 pushes 0 for anim_key; every
// caller passes 0).
class anim_pos_key : public anim_key {
public:
    anim_pos_key(int time = 0, const matrix43 &m = identity_matrix43)   // 0x544ad0 anim_pos_key__ctor_from_matrix
    { pos = m.get_position(); }
    int get_size() { return sizeof(anim_pos_key); }   // 0x544b30 anim_pos_key__get_size
    vector3 pos;                      // +0x04
    vector3 in;                       // +0x10 control points (anim_track__init_tangents for old files)
    vector3 out;                      // +0x1c
};
LAYOUT_CHECK(anim_pos_key, sizeof(anim_pos_key) == 0x28);

// The rotation of a key: four int16 components scaled by 16383 (cquat_dot, cquat_slerp work on it).
// Empty constructor (folded with vector3() at 0x505530).
class cquat {
public:
    cquat() {}
    short x, y, z, w;
};

// Rotation key (0x14 bytes); its implicit constructor is 0x544b40 (anim_key(0), then cquat()).
class anim_rot_key : public anim_key {
public:
    cquat q;                          // +0x04
    signed char tangent[3];           // +0x0c version 0/1 files only (float * 127)
    signed char ease_in;              // +0x0f percent * 2.54
    signed char ease_out;             // +0x10
};
LAYOUT_CHECK(anim_rot_key, sizeof(anim_rot_key) == 0x14);

// Morph key (0x10 bytes); implicit constructor 0x544b70 (vector3()).
struct anim_vec_key {
    short time;                       // +0x0 frame
    vector3 v;                        // +0x4
};
LAYOUT_CHECK(anim_vec_key, sizeof(anim_vec_key) == 0x10);

// One key track of a skeleton (0x2c bytes per bone, skeleton +0x8c). skeleton_read clears it and sets
// the counts and key pointers (into the skeleton's key arrays); the rest is anim_track's own.
class anim_track {
public:
    void init_tangents(int flag);                     // 0x550f90 anim_track__init_tangents (decomp25)
    quaternion get_rotation(int time, int flag);      // 0x551350 (flag 0 in the pilots)
    vector3 get_position(int time);                   // 0x551090
    ushort num_pos_keys;              // +0x00
    ushort num_rot_keys;              // +0x02
    int _unk04;
    anim_pos_key *pos_keys;           // +0x08
    anim_rot_key *rot_keys;           // +0x0c
    ubyte _unk10[0x1c];
};
LAYOUT_CHECK(anim_track, sizeof(anim_track) == 0x2c);

// One morph target track (0x0c bytes, skeleton +0x9c; name ours).
struct skeleton_morph {
    short num_keys;                   // +0x0
    short _unk02;
    anim_vec_key *keys;               // +0x4
    short _unk08;                     // +0x8 set to 1 by skeleton_read
    short _unk0a;
};
LAYOUT_CHECK(skeleton_morph, sizeof(skeleton_morph) == 0xc);

struct ca_trigger {                   // 8 bytes, skeleton +0xa8: "left-foot", "right-foot", ...
    const char *name;                 // +0x0 NULL = unused slot
    int time;                         // +0x4 anim time (ticks)
};

// One animation (.mvf), 0xd0 bytes (sizeof is what skeleton_read / ~skeleton count in skeleton_mem_used).
// character_add_skeleton (character.cpp) allocates it with operator new(0xd0) and the five-argument
// constructor (sum.symbols: skeleton_add), then calls read.
class skeleton {
public:
    skeleton();                                       // 0x543d20 skeleton__skeleton (all zero; no direct call)
    skeleton(const char *filename, const char *name, bool is_state, int unused, bool root_rotation);   // 0x543d50 skeleton_add
    ~skeleton();                                      // 0x543e10 skeleton__dtor
    vector3 get_root_velocity();                      // 0x543fc0 skeleton__get_root_velocity
    quaternion get_root_rotation(float t);            // 0x544010 (no direct call)
    bool read(FileReader *r, const char *name, int path_id);   // 0x544090 skeleton_read (path id unused)

    anim_rot_key *rot_keys;           // +0x00 every track's rotation keys
    anim_pos_key *pos_keys;           // +0x04 every track's position keys
    char filename[0x40];              // +0x08
    int num_rot_keys;                 // +0x48
    int num_pos_keys;                 // +0x4c
    skeleton *next;                   // +0x50 list (unlinked by ~skeleton)
    skeleton *prev;                   // +0x54
    const char *name;                 // +0x58 in ca_string_pool
    bool is_state;                    // +0x5C state (looping, locomotion) anim; 0 = action
    bool root_rotation;               // +0x5D the anim turns the character (yaw taken from the root bone)
    ubyte _pad5e[2];
    int start;                        // +0x60 first tick
    int end;                          // +0x64 last tick
    vector3 root_disp;                // +0x68 root displacement over the anim
    quaternion root_rot;              // +0x74 root rotation over the anim
    int num_bones;                    // +0x84
    ubyte *bone_weights;              // +0x88 per bone, 0..10
    anim_track *tracks;               // +0x8C per bone
    int num_morph_frames;             // +0x90
    int num_morphs;                   // +0x94
    int *morph_ids;                   // +0x98
    skeleton_morph *morphs;           // +0x9c
    int fade_in;                      // +0xA0 ticks
    int fade_out;                     // +0xA4 ticks
    ca_trigger triggers[5];           // +0xA8
};
LAYOUT_CHECK(skeleton, sizeof(skeleton) == 0xd0);

extern int skeleton_count;            // 0x2ee4598
extern int skeleton_count_limit;      // 0x2ee459c (-1 = none)
extern int skeleton_mem_used;         // 0x2ee45a0
extern int skeleton_mem_peak;         // 0x2ee45a4
extern int skeleton_mem_limit;        // 0x2ee45a8 (-1 = none)

// Read the position keys of one track: time and three vectors per key; the reader's error state.
int skeleton_read_tag_record(FileReader *r, anim_pos_key *key);                    // 0x544a50
int skeleton_read_tag_records(FileReader *r, anim_pos_key *keys, unsigned int n);  // 0x544a90 0 = all read

// ---- .vim meshes ------------------------------------------------------------------------------------
// Moved here from character.h (decomp25): the bones and tags of a mesh are part of the .vim data block;
// ca_mesh +0x4 (skin) points at vim +0x38.
struct ca_bone {                      // 0xa0 bytes (ca_skin +0x4)
    ubyte _unk00[0x50];
    matrix43 bind;                    // +0x50 multiplied in front of the animated bone matrix
    int parent;                       // +0x80 -1 = root
    vector3 tag_offset;               // +0x84 character_instance__get_tag_pos
    int tag_value;                    // +0x90
    vector3 base_pos;                 // +0x94 character_instance__get_bone_bind_inverse
};
LAYOUT_CHECK(ca_bone, sizeof(ca_bone) == 0xa0);

struct ca_bone_ext {                  // 0x38 bytes: extra bones (tags) after the real bones
    const char *name;                 // +0x00 in ca_string_pool (vim_read; written by vim_write_header)
    matrix43 m;                       // +0x04
    int parent;                       // +0x34
};
LAYOUT_CHECK(ca_bone_ext, sizeof(ca_bone_ext) == 0x38);

struct ca_skin {                      // mesh +0x4 = vim +0x38
    int num_bones;                    // +0x00
    ca_bone *bones;                   // +0x04
    int _unk08;
    int num_ext;                      // +0x0C
    ca_bone_ext *ext;                 // +0x10
    int root_bone;                    // +0x14
};

// One sub-mesh (0x30 bytes, the first part of the data block). Its arrays follow in the block, each
// 16-byte aligned, sized in bytes by the counts at +0x24..+0x2c; names ours.
struct vim_submesh {
    void *block0;                     // +0x00 size2 bytes
    void *block1;                     // +0x04 size2 bytes
    void *block2;                     // +0x08 size4 bytes
    void *block3;                     // +0x0c size3 bytes
    void *block4;                     // +0x10 size6 bytes
    void *block5;                     // +0x14 size5 bytes (only when size5 > 0)
    int _unk18;
    void *vertex_extra;               // +0x1c 2 bytes per mesh vertex, with VIM_HAS_VERTEX_EXTRA
    ushort size0;                     // +0x20 (read and written, not used for the layout)
    ushort size1;                     // +0x22
    ushort size2;                     // +0x24
    ushort size3;                     // +0x26
    ushort size4;                     // +0x28
    ushort size5;                     // +0x2a
    ushort size6;                     // +0x2c
    ushort _pad2e;
};
LAYOUT_CHECK(vim_submesh, sizeof(vim_submesh) == 0x30);

// A mesh tag (0x60 bytes, vim +0x60): name, orientation, position.
struct vim_tag {
    char name[0x44];                  // +0x00 vim_find_tag (_stricmp)
    quaternion orient;                // +0x44
    vector3 pos;                      // +0x54
};
LAYOUT_CHECK(vim_tag, sizeof(vim_tag) == 0x60);

#define VIM_VERSION          6        // the version vim_write_header writes and vim_load_file needs
#define VIM_HAS_VERTEX_EXTRA 1        // vim flags: vim_submesh::vertex_extra present
#define VIM_IN_MEMPOOL       8        // vim flags: allocated while the mempool was active (vim_free skips it)

// The .vim file header (0x24 bytes; vim_read_header, vim_read).
struct vim_header {
    vim_header();                     // 0x543d10 vim_header__ctor
    int version;                      // +0x00
    int flags;                        // +0x04 or-ed into vim::flags
    int num_vertices;                 // +0x08
    ushort num_submeshes;             // +0x0c
    vector3 center;                   // +0x10
    float radius;                     // +0x1c
    int data_size;                    // +0x20
};
LAYOUT_CHECK(vim_header, sizeof(vim_header) == 0x24);

// The .vim mesh (0x68 bytes; item meshes: item_paging.cpp; character meshes: ca_mesh +0x28). One type
// for the stubs' vim and ca_vim (DECISIONS.md D21).
struct vim {
    vim();                            // 0x543d00 vim__vim
    int data_size;                    // +0x00
    ubyte *data;                      // +0x04 one block: sub-meshes, their arrays, bones, tags
    vim_submesh *submeshes;           // +0x08 = data
    ushort num_submeshes;             // +0x0c
    ushort _pad0e;
    int textures[6];                  // +0x10 bitmap handles
    int num_textures;                 // +0x28
    int flags;                        // +0x2c VIM_*
    int num_vertices;                 // +0x30
    int _unk34;                       // +0x34
    ca_skin skin;                     // +0x38 bones (0xa0 each) and named extra bones (0x38 each)
    vector3 center;                   // +0x50 bounding sphere
    float radius;                     // +0x5C
    vim_tag *tags;                    // +0x60
    int num_tags;                     // +0x64
};
LAYOUT_CHECK(vim, sizeof(vim) == 0x68);
typedef vim ca_vim;

vim *vim_alloc();                                  // 0x543040
void vim_free(vim *mesh);                          // 0x5430b0
int  vim_write_cache(vim *mesh, const char *name, int path_id);   // 0x5430e0 1 = written
int  vim_write_header(vim *mesh, File *f);         // 0x543230 (the whole mesh; 0 when a texture has no file name)
void vim_read_header(FileReader *r, vim_header *hdr);              // 0x5433d0
int  vim_read(FileReader *r, vim *mesh, int reload, int bm_arg, vim_header *hdr);   // 0x543430 1 = read
vim *vim_load_file(FileReader *r, int bm_arg);     // 0x5438b0
vim *vim_load(const char *name, int arg);          // 0x543920
void vim_reload_bitmaps(const char *name, vim *mesh);   // 0x543a80
void vim_copy_bitmaps(vim *dst, const vim *src);   // 0x543bb0
void vim_save_bitmaps(const vim *mesh);            // 0x543bd0
void vim_restore_bitmaps(vim *mesh);               // 0x543c00
int  vim_find_tag(vim *mesh, const char *name);    // 0x543c20, -1 if none
void vim_get_tag_transform(vim *mesh, int index, vector3 *pos, matrix *orient);   // 0x543c70
int  vim_bitmaps_loaded(vim *mesh);                // 0x543cd0
void ca_init_globals_2ee4598();                    // 0x544a10
void ca_close_globals_544a40();                    // 0x544a40 (no name in sum.symbols; label ours)

#endif
