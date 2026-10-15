// vsdk/ca/character.h - the base animated character and its skinned mesh (vsdk\ca\character.cpp, path
// references 0x5409bc-0x541560; ca_render_* 0x50d6e0 is vsdk\ca in sum.symbols, file not known).
// Not original source. Field names ours unless a symbol comment names them.
#ifndef VSDK_CA_CHARACTER_H
#define VSDK_CA_CHARACTER_H

#include "vsdk/ca/skeleton.h"
#include "vsdk/gr/gr.h"
#include "vsdk/os/stringpool.h"

// ca_bone, ca_bone_ext and ca_skin moved to skeleton.h (decomp25: they live in the .vim data block,
// ca_skin = vim +0x38).

// The animation and bone-tag names (skeleton names, trigger names, ca_bone_ext names).
extern StringPool ca_string_pool;     // 0x2dc7070 (character.cpp .bss; decomp25)

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

// (names, num_names, a3, a4, model pool, char pool): finds or creates the base character named
// names[0]. GameInit passes (&hero_model_names[2k], 2 (Rosalind 1), 1, 0, pools). a3 / a4 unnamed.
class Mempool;
ca_character *character_load(char **names, int num_names, int a3, int a4, Mempool *model_pool,
                             Mempool *char_pool);     // 0x541450 (return unused by GameInit)

// Mesh rendering used by the 3D views of the screens.
void ca_render_set_mesh(void *mesh);                                       // 0x50d6e0
void ca_render_mesh(vector3 *pos, matrix *orient, gr_light_desc *light);   // 0x50d6f0 (return unused)

#endif
