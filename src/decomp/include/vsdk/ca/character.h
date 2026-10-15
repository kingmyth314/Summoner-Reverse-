// vsdk/ca/character.h - the base animated character and its meshes (vsdk\ca\character.cpp, code
// 0x540810-0x54195f, path references 0x5409bc-0x541560; ca_render_* 0x50d6e0 is vsdk\ca in sum.symbols,
// file not known). Not original source. Field names ours unless a symbol comment names them.
// decomp30: character.cpp written; the layouts below are completed from it (DECISIONS.md D1000-D1004).
#ifndef VSDK_CA_CHARACTER_H
#define VSDK_CA_CHARACTER_H

#include "vsdk/ca/skeleton.h"
#include "vsdk/gr/gr.h"
#include "vsdk/os/stringpool.h"

class Mempool;
struct ca_character;

// ca_bone, ca_bone_ext and ca_skin moved to skeleton.h (decomp25: they live in the .vim data block,
// ca_skin = vim +0x38).

// The animation and bone-tag names (skeleton names, trigger names, ca_bone_ext names).
extern StringPool ca_string_pool;     // 0x2dc7070 (character.cpp .bss; decomp25)

#define CA_MAX_CHARACTERS   200       // ca_characters ("MAX_BASE_CHARACTERS" in the error text)
#define CA_MAX_SKELETONS    100       // ca_character::skeletons ("No more room for skeletons")
#define CA_MAX_MESHES       205       // ca_preload_slots (0x2ddfa48-0x2de20b7, 0x30 bytes each)
#define CA_NUM_LODS         2         // ca_character::lods
#define CA_NUM_TRIGGERS     5         // skeleton::triggers (= CI_NUM_TRIGGERS)
#define CA_MAX_PRELOAD      70        // ca_preload_offsets / ca_preload_sizes
#define CA_PRELOAD_DATA_SIZE 0x100000 // ca_preload_data (0x2de20c4-0x2ee20c3)

// ca_character::flags (names ours).
#define CA_LOADED           0x01      // slot in use (character_create_base; ca_close, character_release_all)
#define CA_KEEP_LOADED      0x02      // character_load(keep_loaded): character_release_all skips it
#define CA_MESH_LOADED      0x04      // set by load_shadow
#define CA_IN_MEMPOOL       0x08      // created while a mempool was active: skeletons and meshes are not freed
#define CA_SKELETONS_LOCKED 0x10      // lock_skeleton_count saved the count

// Per-vertex bone weights (8 bytes: four bone indices, four weights; name from sum.symbols). The
// functions are in character.cpp, out of line, with no caller in Sum.exe (DECISIONS.md D1002).
struct ca_vertex_weights {
    ca_vertex_weights();                              // 0x5408f0 (sum.symbols ca_vertex_weights__clear; returns this)
    ~ca_vertex_weights();                             // 0x540900 (bare ret)
    void add(ubyte bone, ubyte weight);               // 0x540910 first free index
    void clear_b(int a, int b);                       // 0x540940 (arguments unused)
    ubyte bones[4];                   // +0x0 0 = free
    ubyte weights[4];                 // +0x4
};

// A mesh slot (0x30 bytes; ca_preload_slots, the 205-entry table after ca_characters). The current LOD
// of a character is character +0x1e4, the LOD slots +0x1e8 (D1001).
struct ca_mesh {
    ca_character *owner;              // +0x00 NULL = free slot
    ca_skin *skin;                    // +0x04 = &vim->skin
    char filename[0x20];              // +0x08 the stripped name (load_shadow loads it)
    ca_vim *vim;                      // +0x28
    ca_vim *shadow;                   // +0x2c name_shadow.vim, NULL if none
};
LAYOUT_CHECK(ca_mesh, sizeof(ca_mesh) == 0x30);

// The base character (0x1f8 bytes, ca_characters[200]); the instance points at it (+0x3c). The
// constructor and destructor are empty and out of line in character.cpp (the static array is built with
// the EH vector constructor). The thiscall members are flat character_* names in sum.symbols (D1000).
struct ca_character {
    ca_character();                                   // 0x540950 ca_character__ca_character
    ~ca_character();                                  // 0x540960 ca_character__dtor

    int  add_skeleton(const char *filename, const char *name, bool is_state, int unused,
                      bool check_dup, bool root_rotation);   // 0x540970 character_add_skeleton, index
    void free_skeletons();                            // 0x540b80 character_free_skeletons
    void free_skeleton(int index);                    // 0x540bd0 character_free_skeleton
    void lock_skeleton_count();                       // 0x540c10 character_lock_skeleton_count
    void unlock_skeleton_count();                     // 0x540c30 character_unlock_skeleton_count
    void add_skeleton_trigger(int anim, const char *name, float frame);       // 0x540c80 character_add_skeleton_trigger
    void ensure_skeleton_trigger(int anim, const char *name, float frame);    // 0x540cf0 character_ensure_skeleton_trigger
    void set_skeleton_trigger_time(int anim, const char *name, float frame);  // 0x540d70 character_set_skeleton_trigger_time
    bool load_shadow(const char *name, ca_mesh *mesh);   // 0x540df0 character_load_shadow (loads the mesh too)
    int  find_state_skeleton(const char *name);       // 0x540ed0 character_find_state_skeleton, -1 if none
    int  find_action_skeleton(const char *name);      // 0x540f20 character_find_action_skeleton, -1 if none
    int  find_mesh_tag(const char *name);             // 0x540f70 character_find_mesh_tag: bone or extra bone, -1
    int  report_memory();                             // 0x541020 character_report_memory
    void reload_mesh(int lod);                        // 0x541160 character_reload_mesh
    void replace_texture(int index, const char *file);   // 0x541240 character_replace_texture
    int  get_lod_index();                             // 0x541270 character_get_lod_index, -1 if none

    char name[0x44];                  // +0x000 printed with %s (stripped file name)
    int flags;                        // +0x044 CA_*
    int num_anims;                    // +0x048 skeletons in use
    int saved_num_anims;              // +0x04c lock_skeleton_count
    int _unk050;                      // +0x050 cleared by character_clear only
    skeleton *skeletons[CA_MAX_SKELETONS];   // +0x054
    ca_mesh *mesh;                    // +0x1E4 current LOD
    ca_mesh *lods[CA_NUM_LODS];       // +0x1E8
    int _unk1f0;                      // +0x1F0 cleared by character_clear only (a third LOD slot? D1001)
    Mempool *pool;                    // +0x1F4 model pool (reload_mesh resets it)
};
LAYOUT_CHECK(ca_character, sizeof(ca_character) == 0x1f8);

// The preload index (ca_load_preload_index): the whole file is read into ca_preload_data; its header is
// two ints, the entry count and the data size, then 0x48-byte entries (names ours, D1003).
struct ca_preload_entry {
    char name[0x40];                  // +0x00 ca_preload_find (_stricmp)
    int offset;                       // +0x40 from the start of the file
    int size;                         // +0x44
};
LAYOUT_CHECK(ca_preload_entry, sizeof(ca_preload_entry) == 0x48);
struct ca_preload_header {
    int _unk00;
    int _unk04;
    int count;                        // +0x08
    int size;                         // +0x0c
    ca_preload_entry entries[1];      // +0x10
};

extern ca_character ca_characters[CA_MAX_CHARACTERS];   // 0x2dc7088
extern ca_mesh ca_preload_slots[CA_MAX_MESHES];          // 0x2ddfa48 (the mesh slot table)
extern int ca_preload_loaded;                            // 0x2de20b8 (level_load_preload clears it)
extern int ca_preload_count;                             // 0x2de20bc
extern int ca_preload_size;                              // 0x2de20c0
extern ubyte ca_preload_data[CA_PRELOAD_DATA_SIZE];      // 0x2de20c4
extern int ca_preload_offsets[CA_MAX_PRELOAD];           // 0x2ee20c4 (name ours)
extern int ca_preload_sizes[CA_MAX_PRELOAD];             // 0x2ee21dc (name ours)
extern int ca_num_characters;                            // 0x2ee24f0
extern ca_character *ca_current_character;               // 0x2ee45ac (name ours; written by add_skeleton only)

void ca_init(int pool_size);          // 0x541290 (-1 = 0x1400 bytes of names)
void character_clear(ca_character *c);   // 0x541300
void ca_close();                      // 0x541330
void character_unload(ca_character *c);  // 0x541360
ca_character *ca_find_character(const char *name);   // 0x541400 NULL if none

// (names, num_names, keep_loaded, unused, model pool, char pool): finds or creates the base character
// named names[0]; names[1..] are the other LOD meshes. GameInit passes (&hero_model_names[2k], 2
// (Rosalind 1), 1, 0, pools). keep_loaded is compared as a byte (D1004).
ca_character *character_load(char **names, int num_names, bool keep_loaded, int unused,
                             Mempool *model_pool, Mempool *char_pool);   // 0x541450
ca_character *character_create_base(char **names, int num_names, Mempool *model_pool,
                                    Mempool *char_pool);                 // 0x541500
ca_mesh *ca_preload_alloc_slot();     // 0x541700 NULL when full
void character_release(ca_character *c);  // 0x541740
void character_release_all();         // 0x541760
void ca_load_preload_index(const char *name);   // 0x541790
int  ca_preload_find(const char *name, int *offset, int *size);   // 0x5418f0 1 = found

// Mesh rendering used by the 3D views of the screens.
// decomp60 (character_instance::render): the character and instance the next ca_render_mesh uses
// (cleared with (NULL, NULL) afterwards).
class character_instance;
void ca_render_set_instance(ca_character *character, character_instance *instance);   // 0x50d6c0
void ca_render_set_mesh(void *mesh);                                       // 0x50d6e0
void ca_render_mesh(vector3 *pos, matrix *orient, gr_light_desc *light);   // 0x50d6f0 (return unused)

#endif
