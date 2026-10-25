// vsdk/ca/skeleton.h - animations (.mvf skeletons), key tracks and .vim meshes (vsdk\ca\skeleton.cpp,
// path references 0x543d8c-0x5449da; anim_track 0x550660-0x5519b9 is in an unnamed unit, decomp70). Not original
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

// The rotation of a key: cquat, four int16 components scaled by 16383 (decomp60: declared in
// vsdk/math/quaternion.h, the header of the unit with its members, which matrix.h includes).

// Rotation key (0x14 bytes); its implicit constructor is 0x544b40 (anim_key(0), then cquat()).
class anim_rot_key : public anim_key {
public:
    cquat q;                          // +0x04
    signed char tangent[3];           // +0x0c tension, continuity, bias (float * 127; decomp70)
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
// decomp70: the anim_track unit is vsdk/unnamed_anim_track_0x550660.cpp (0x550660-0x5519b9, built
// without inline expansion). The small accessors below are inline: their only copies are the COMDATs
// at the end of that object (0x551770-0x55195d, in this order). The rotation key bytes +0x0c..+0x0e
// are the tension, continuity and bias (tangent[0..2]) and +0x0f / +0x10 the ease to / ease from
// (ease_in / ease_out), all read as value / 127 (0x581eb8). The track also has scale keys (+0x04
// count, +0x10 keys, +0x24 / +0x28 their control points; D5101); the vfx mesh code (anim_track
// get_scale, relocate) is their only user.
class anim_track {
public:
    // decomp70 (D5103): both flags are bools (passed on as dwords, tested as bytes); were int.
    void init_tangents(bool loop);                    // 0x550f90 anim_track__init_tangents (decomp25)
    quaternion get_rotation(int time, bool squad);    // 0x551350 (0 for skeletons, 1 for the cutscene camera)
    vector3 get_position(int time);                   // 0x551090
    // decomp70 (the rest of the unit).
    void calc_pos_controls(int key);                  // 0x550660 control b = value + out, a = value + in
    void calc_rot_tangents(int key, bool loop);       // 0x5506e0 the squad inner quaternions (TCB)
    void calc_scale_controls(int key);                // 0x550d10
    void eval_scale_bezier(vector3 *out, int k0, int k1, float t);   // 0x550d90
    void get_rot_time_range(int *first, int *last);   // 0x550f60 first and last rotation key time
    vector3 get_scale(int time);                      // 0x551520 (vfx_mesh_instance::update_frame)
    void relocate(char *base);                        // 0x551700 the nine pointers (vfx_mesh::relocate)

    vector3 *pos_key_value(int i) { return &pos_keys[i].pos; }       // 0x551770
    vector3 *pos_key_in(int i) { return &pos_keys[i].in; }           // 0x551790
    vector3 *pos_key_out(int i) { return &pos_keys[i].out; }         // 0x5517b0
    vector3 *pos_control_a(int i) { return &pos_aux_a[i]; }          // 0x5517d0
    vector3 *pos_control_b(int i) { return &pos_aux_b[i]; }          // 0x5517e0
    int rot_key_time(int i) { return rot_keys[i].time; }             // 0x5517f0
    cquat *rot_key_quat(int i) { return &rot_keys[i].q; }            // 0x551800
    float rot_key_continuity(int i) { return rot_keys[i].tangent[1] * (1.0f / 127.0f); }   // 0x551820
    float rot_key_tension(int i) { return rot_keys[i].tangent[0] * (1.0f / 127.0f); }      // 0x551840
    float rot_key_bias(int i) { return rot_keys[i].tangent[2] * (1.0f / 127.0f); }         // 0x551860
    cquat *rot_inner_a(int i) { return &rot_aux_a[i]; }              // 0x551880
    cquat *rot_inner_b(int i) { return &rot_aux_b[i]; }              // 0x551890
    vector3 *scale_key_value(int i) { return &scale_keys[i].pos; }   // 0x5518a0
    vector3 *scale_key_in(int i) { return &scale_keys[i].in; }       // 0x5518c0
    vector3 *scale_key_out(int i) { return &scale_keys[i].out; }     // 0x5518e0
    vector3 *scale_control_a(int i) { return &scale_aux_a[i]; }      // 0x551900
    vector3 *scale_control_b(int i) { return &scale_aux_b[i]; }      // 0x551910
    float rot_key_ease_to(int i) { return rot_keys[i].ease_in * (1.0f / 127.0f); }         // 0x551920
    float rot_key_ease_from(int i) { return rot_keys[i].ease_out * (1.0f / 127.0f); }      // 0x551940

    ushort num_pos_keys;              // +0x00
    ushort num_rot_keys;              // +0x02
    ushort num_scale_keys;            // +0x04 (decomp70; was int _unk04)
    ushort _unk06;
    anim_pos_key *PTR32 pos_keys;           // +0x08
    anim_rot_key *PTR32 rot_keys;           // +0x0c
    // decomp54 (cutscene_camera_read_track hands these out from its pools, one entry per key; D3402):
    // two 0xc-byte arrays per position key and two 8-byte arrays per rotation key (was _unk10[0x1c]).
    // Names ours. decomp70: pos_aux_a / _b are the Bezier control points a (before the key) and b
    // (after it), rot_aux_a / _b the squad inner quaternions.
    anim_pos_key *PTR32 scale_keys;         // +0x10 (decomp70; was int _unk10) same 0x28-byte layout
    vector3 *PTR32 pos_aux_a;               // +0x14
    vector3 *PTR32 pos_aux_b;               // +0x18
    cquat *PTR32 rot_aux_a;                 // +0x1c
    cquat *PTR32 rot_aux_b;                 // +0x20
    vector3 *PTR32 scale_aux_a;             // +0x24 (decomp70; was ubyte _unk24[8])
    vector3 *PTR32 scale_aux_b;             // +0x28
};
LAYOUT_CHECK(anim_track, sizeof(anim_track) == 0x2c);
// (t, ease_from, ease_to): the 3D Studio style ease of a key interval (cdecl; decomp54, the camera blend).
float tcb_ease(float t, float ease_from, float ease_to);   // 0x551610

// One morph target track (0x0c bytes, skeleton +0x9c; name ours).
struct skeleton_morph {
    short num_keys;                   // +0x0
    short _unk02;
    anim_vec_key *PTR32 keys;               // +0x4
    short _unk08;                     // +0x8 set to 1 by skeleton_read
    short _unk0a;
};
LAYOUT_CHECK(skeleton_morph, sizeof(skeleton_morph) == 0xc);

struct ca_trigger {                   // 8 bytes, skeleton +0xa8: "left-foot", "right-foot", ...
    const char *PTR32 name;                 // +0x0 NULL = unused slot
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

    anim_rot_key *PTR32 rot_keys;           // +0x00 every track's rotation keys
    anim_pos_key *PTR32 pos_keys;           // +0x04 every track's position keys
    char filename[0x40];              // +0x08
    int num_rot_keys;                 // +0x48
    int num_pos_keys;                 // +0x4c
    skeleton *PTR32 next;                   // +0x50 list (unlinked by ~skeleton)
    skeleton *PTR32 prev;                   // +0x54
    const char *PTR32 name;                 // +0x58 in ca_string_pool
    bool is_state;                    // +0x5C state (looping, locomotion) anim; 0 = action
    bool root_rotation;               // +0x5D the anim turns the character (yaw taken from the root bone)
    ubyte _pad5e[2];
    int start;                        // +0x60 first tick
    int end;                          // +0x64 last tick
    vector3 root_disp;                // +0x68 root displacement over the anim
    quaternion root_rot;              // +0x74 root rotation over the anim
    int num_bones;                    // +0x84
    ubyte *PTR32 bone_weights;              // +0x88 per bone, 0..10
    anim_track *PTR32 tracks;               // +0x8C per bone
    int num_morph_frames;             // +0x90
    int num_morphs;                   // +0x94
    int *PTR32 morph_ids;                   // +0x98
    skeleton_morph *PTR32 morphs;           // +0x9c
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
    char name[0x50];                  // +0x00 character_find_mesh_tag (_stricmp; length ours, decomp30 D1001)
    matrix43 bind;                    // +0x50 multiplied in front of the animated bone matrix
    int parent;                       // +0x80 -1 = root
    vector3 tag_offset;               // +0x84 character_instance__get_tag_pos
    int tag_value;                    // +0x90
    vector3 base_pos;                 // +0x94 character_instance__get_bone_bind_inverse
};
LAYOUT_CHECK(ca_bone, sizeof(ca_bone) == 0xa0);

struct ca_bone_ext {                  // 0x38 bytes: extra bones (tags) after the real bones
    const char *PTR32 name;                 // +0x00 in ca_string_pool (vim_read; written by vim_write_header)
    matrix43 m;                       // +0x04
    int parent;                       // +0x34
};
LAYOUT_CHECK(ca_bone_ext, sizeof(ca_bone_ext) == 0x38);

struct ca_skin {                      // mesh +0x4 = vim +0x38
    int num_bones;                    // +0x00
    ca_bone *PTR32 bones;                   // +0x04
    int _unk08;
    int num_ext;                      // +0x0C
    ca_bone_ext *PTR32 ext;                 // +0x10
    int root_bone;                    // +0x14
};

// One sub-mesh (0x30 bytes, the first part of the data block). Its arrays follow in the block, each
// 16-byte aligned, sized in bytes by the counts at +0x24..+0x2c; names ours.
struct vim_submesh {
    void *PTR32 block0;                     // +0x00 size2 bytes
    void *PTR32 block1;                     // +0x04 size2 bytes
    void *PTR32 block2;                     // +0x08 size4 bytes
    void *PTR32 block3;                     // +0x0c size3 bytes
    void *PTR32 block4;                     // +0x10 size6 bytes
    void *PTR32 block5;                     // +0x14 size5 bytes (only when size5 > 0)
    int _unk18;
    void *PTR32 vertex_extra;               // +0x1c 2 bytes per mesh vertex, with VIM_HAS_VERTEX_EXTRA
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
    ubyte *PTR32 data;                      // +0x04 one block: sub-meshes, their arrays, bones, tags
    vim_submesh *PTR32 submeshes;           // +0x08 = data
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
    vim_tag *PTR32 tags;                    // +0x60
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
