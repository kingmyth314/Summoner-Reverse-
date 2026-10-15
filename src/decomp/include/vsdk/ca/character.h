// vsdk/ca/character.h - the base animated character and its skinned mesh (vsdk\ca\character.cpp, path
// references 0x5409bc-0x541560; ca_render_* 0x50d6e0 is vsdk\ca in sum.symbols, file not known).
// Not original source. Field names ours unless a symbol comment names them.
#ifndef VSDK_CA_CHARACTER_H
#define VSDK_CA_CHARACTER_H

#include "vsdk/ca/skeleton.h"
#include "vsdk/gr/gr.h"

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
    int _unk00;
    matrix43 m;                       // +0x04
    int parent;                       // +0x34
};
LAYOUT_CHECK(ca_bone_ext, sizeof(ca_bone_ext) == 0x38);

struct ca_skin {                      // mesh +0x4
    int num_bones;                    // +0x00
    ca_bone *bones;                   // +0x04
    int _unk08;
    int num_ext;                      // +0x0C
    ca_bone_ext *ext;                 // +0x10
    int root_bone;                    // +0x14
};

struct ca_mesh {                      // character +0x1e4 (current LOD)
    int _unk00;
    ca_skin *skin;                    // +0x04
    ubyte _unk08[0x20];
    ca_vim *vim;                      // +0x28
};

// The base character; the instance points at it (+0x3c).
struct ca_character {
    char name[0x48];                  // +0x000 printed with %s (size up to the next field, ours)
    int num_anims;                    // +0x048
    ubyte _unk04c[8];
    skeleton *skeletons[100];         // +0x054
    ca_mesh *mesh;                    // +0x1E4
};
LAYOUT_CHECK(ca_character, sizeof(ca_character) == 0x1e8);

void ca_init(int arg);                // 0x541290
void ca_close();                      // 0x541330

// Mesh rendering used by the 3D views of the screens.
void ca_render_set_mesh(void *mesh);                                       // 0x50d6e0
void ca_render_mesh(vector3 *pos, matrix *orient, gr_light_desc *light);   // 0x50d6f0 (return unused)

#endif
