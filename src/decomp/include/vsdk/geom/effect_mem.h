// effect_mem.h - Sum.exe v1.40 effect bases and effect memory pools (vsdk\geom\effect_mem.cpp,
// code 0x51bb70-0x51e018). Written from the Sum.exe disassembly. Names from symbols/sum.symbols;
// record field names are ours where the symbols give only offsets.
#ifndef EFFECT_MEM_H
#define EFFECT_MEM_H

#include "vsdk/os/memory.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/vfile/file.h"
#include "vsdk/math/matrix.h"
#include "vsdk/geom/vfx.h"

#define MAX_EFFECT_BASES     192      // effect_bases, 0xf00 bytes
#define MAX_EFFECT_NAMES     440      // effect_base_names, 0x1b8 entries
#define MAX_ARR_NAMES        32       // names per effects.arr line (the parser can write one more)
#define ARR_NAME_LEN         41       // 0x29

#define NUM_STREAM_SLOTS     8        // effects_stream_slots (scanned 6 at a time, see game/NOTES-player-effectmem.md)
#define NUM_BIG_SLOTS        6        // effects_stream_slots_b
#define STREAM_SLOT_SIZE     0x3d0c0  // 250048
#define BIG_SLOT_SIZE        0x7a140  // 500032
#define EFFECTS_PRELOAD_SIZE 0x4c4c0  // preload.rfx area at the start of effects_mem_buffer
#define EFFECTS_CUTSCENE_SIZE (NUM_STREAM_SLOTS * STREAM_SLOT_SIZE + NUM_BIG_SLOTS * BIG_SLOT_SIZE) // 0x4c4d80

// effects.arr categories (effect_base.category)
#define EFFECT_CAT_LEVEL     0
#define EFFECT_CAT_PRELOAD   1
#define EFFECT_CAT_SPELL     2
#define EFFECT_CAT_CUTSCENE  3
#define EFFECT_CAT_SUMMON    4
#define EFFECT_CAT_CHARACTER 5

// One entry of a base's node table (data +0, 8 bytes): name hash and node pointer.
struct vfx_node_entry {
    int hash;                         // +0 effects_hash_name of the effect name
    vfx_base *base;                   // +4 relocated by ptr_relocate
    void relocate(char *base_addr, char *end);   // 0x51ca30 vfx_relocate_entry
};

// An effect base record (0x14 bytes, effect_bases 0x2c8f060). The first 12 bytes have the layout of
// the .vfx/.rfx file header read by effect_base_read. +8 is a bitfield word: the 23-bit size store
// is the XOR/AND/XOR sequence VC6 emits for bitfields, and the category is read both as a mask
// compare and as SHR 27 / AND 7.
struct effect_base {
    ushort num_nodes;                 // +0x00 entries of the node table at data
    ushort version;                   // +0x02 must be 9 (effect_base_load)
    int table_size;                   // +0x04 bytes before the node data (relocation end = data + this)
    unsigned int size : 23;           // +0x08 bits 0-22
    unsigned int level : 1;           //       bit 23, cleared per level and by rfx_preload
    unsigned int bitmaps_pending : 1; //       bit 24, set when streamed in, bitmaps loaded on release
    unsigned int big_slot : 1;        //       bit 25, data in a BIG_SLOT_SIZE slot
    unsigned int cutscene : 1;        //       bit 26
    unsigned int category : 3;        //       bits 27-29 EFFECT_CAT_*
    unsigned int streamed : 1;        //       bit 30
    unsigned int resident : 1;        //       bit 31
    char *data;                       // +0x0c
    const char *name;                 // +0x10 string pool
    void relocate(char *base_addr, char *end);   // 0x51c9f0 vfx_relocate
};
LAYOUT_CHECK(effect_base, sizeof(effect_base) == 0x14);

// Effect record pools threaded by effects_init_pools. Only the constructed members and the links are
// named; the constructors are the compiler-generated ones (0x51df10-0x51dfee; the three
// two-vector types share one body at 0x51df60, folded by the linker).
struct fx_rec48 {                     // vfx instance (0x48), free list vfx_instance_free_list
    unsigned int flags;               // +0x00 bit 0x40 cleared by effects_init_pools
    char _unk04[0x18 - 0x04];
    int _unk18, _unk1c, _unk20;       // +0x18..+0x20 zeroed by effects_init_pools
    int _unk24;
    int _unk28;                       // +0x28 zeroed by effects_init_pools
    char _unk2c[0x30 - 0x2c];
    vector3 pos;                      // +0x30
    char _unk3c[0x40 - 0x3c];
    fx_rec48 *next;                   // +0x40
    fx_rec48 *prev;                   // +0x44
};
struct fx_rec98 {
    vector3 v00;                      // +0x00
    char _unk0c[0x20 - 0x0c];
    vector3 v20;                      // +0x20
    matrix m2c;                       // +0x2c
    vector3 v50;                      // +0x50
    matrix m5c;                       // +0x5c
    vector3 v80;                      // +0x80
    char _unk8c[0x90 - 0x8c];
    fx_rec98 *next;                   // +0x90
    fx_rec98 *prev;                   // +0x94
};
struct fx_rec30 {
    char _unk00[0x08];
    vector3 v08, v14;                 // +0x08, +0x14
    char _unk20[0x28 - 0x20];
    fx_rec30 *next;                   // +0x28
    fx_rec30 *prev;                   // +0x2c
};
struct fx_rec2c {
    char _unk00[0x08];
    vector3 v08, v14;
    char _unk20[0x24 - 0x20];
    fx_rec2c *next;                   // +0x24
    fx_rec2c *prev;                   // +0x28
};
struct fx_rec28 {
    char _unk00[0x08];
    vector3 v08, v14;
    fx_rec28 *next;                   // +0x20
    fx_rec28 *prev;                   // +0x24
};
struct fx_rec4c {
    char _unk00[0x0c];
    vector3 v0c, v18;                 // +0x0c, +0x18
    char _unk24[0x28 - 0x24];
    vector3 v28, v34;                 // +0x28, +0x34
    char _unk40[0x44 - 0x40];
    fx_rec4c *next;                   // +0x44
    fx_rec4c *prev;                   // +0x48
};
struct fx_recb0 {
    char _unk00[0x1c];
    vector3 v1c;                      // +0x1c
    matrix m28;                       // +0x28
    vector3 v4c, v58, v64, v70, v7c;  // +0x4c..+0x7c
    char _unk88[0xa8 - 0x88];
    fx_recb0 *next;                   // +0xa8
    fx_recb0 *prev;                   // +0xac
};
LAYOUT_CHECK(fx_rec48, sizeof(fx_rec48) == 0x48);
LAYOUT_CHECK(fx_rec98, sizeof(fx_rec98) == 0x98);
LAYOUT_CHECK(fx_rec30, sizeof(fx_rec30) == 0x30);
LAYOUT_CHECK(fx_rec2c, sizeof(fx_rec2c) == 0x2c);
LAYOUT_CHECK(fx_rec28, sizeof(fx_rec28) == 0x28);
LAYOUT_CHECK(fx_rec4c, sizeof(fx_rec4c) == 0x4c);
LAYOUT_CHECK(fx_recb0, sizeof(fx_recb0) == 0xb0);

extern effect_base effect_bases[MAX_EFFECT_BASES];      // 0x2c8f060
extern const char *effect_base_names[MAX_EFFECT_NAMES]; // 0x27281d8
extern ubyte effect_base_record_index[MAX_EFFECT_NAMES];// 0x2c8e7c8
extern ubyte effect_base_slot[MAX_EFFECT_NAMES];        // 0x2722820
extern ubyte effect_base_stream_users[MAX_EFFECT_BASES];// 0x2c9f1b0
extern int effects_stream_slots[NUM_STREAM_SLOTS];      // 0x27288b8
extern int effects_stream_slots_saved[NUM_STREAM_SLOTS];// 0x272b8dc
extern int effects_stream_slots_b[NUM_BIG_SLOTS];       // 0x2c9f270
extern int effects_stream_slots_b_saved[NUM_BIG_SLOTS]; // 0x2c8e7b0
extern int effects_num_bases;                           // 0x2c9f998
extern int effects_num_names;                           // 0x2c9f99c
extern int effects_preload_count;                       // 0x2c9f96c
extern char effects_mem_buffer[EFFECTS_PRELOAD_SIZE + EFFECTS_CUTSCENE_SIZE]; // 0x2731450
#define effects_stream_buffer (effects_mem_buffer + EFFECTS_PRELOAD_SIZE)    // 0x277d910
extern Mempool effects_mempool;                         // 0x2c98a70
extern StringPool effects_string_pool;                  // 0x2c7dd98

// written in effect_mem.cpp
void cmd_show_effect_mem();
char *effect_base_alloc_stream_buffer(int index);
int align_up_64(int value);
void effect_base_finish_stream(int index);
int vfx_preload_by_name(const char *name);
int vfx_find_name(const char *path);
int rfx_preload(int index);
void effect_base_release(int index);
int effect_base_load(int index);
int effects_alloc_stream_slot(int base);
int effects_find_stream_slot(int base);
char *effects_preload_slot_ptr(int slot);
char *effects_stream_slot_ptr(int slot);
int effect_base_queue_stream(int index, unsigned int *out_size, unsigned int *out_offset, bool queue, Mempool *pool);
vfx_base *effect_get_base(int id);
int effects_get_num_bases();
int effects_find_base(const char *name);
void effect_base_set_info(unsigned int index, ushort num_nodes, int table_size, int size, ushort version);
void effect_base_read(unsigned int index, int *out_nodes, int *out_table_size, int *out_size, int *out_version);
vfx_base *vfx_preload(const char *name, int *out_index);
int effects_parse_arr_line(File *f, int *line, char *name, int *category, unsigned int *num,
                           char names[][ARR_NAME_LEN]);
void effects_reset_level();
void effects_init_pools();
int effects_load_bases();
void effects_show_memory_usage();

// same unit, not written yet (0x51d250-0x51df0f, see game/NOTES-player-effectmem.md)
int effects_hash_name(const char *name);                // 0x51db50
void effects_load_preload_rfx();                        // 0x51dbb0 (level_preload_effects tail-jumps to it; decomp29)

void effects_release_level_memory();  // 0x51dc20 (level_unload; decomp42)
void effects_release_cutscene_memory();   // 0x51def0 (cutscene_cleanup; decomp52)
// decomp48 (the interface manager's model lights "inv-lights1.vfx"): an instance of an effect file and
// its release. vfx_load's second argument is 1 at that call (meaning not known).
void vfx_instance_free(vfx_instance *vi);                // 0x51d7b0
vfx_instance *vfx_load(const char *name, int a);        // 0x51dd00

#endif
