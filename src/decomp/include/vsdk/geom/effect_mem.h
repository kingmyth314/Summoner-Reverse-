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
// decomp60 (D4000): the 0x48-byte record is the vfx instance and the 0x30-byte one the instance light,
// declared in vfx.h; their compiler-generated constructors are 0x51df10 and the shared 0x51df60.
typedef vfx_instance fx_rec48;        // free list vfx_instance_free_list
typedef vfx_light_desc fx_rec30;      // free list fx_rec30_free_list
// A node record of an instance (0x98; vfx_mesh_render's mesh instance, the pointer at 0x2c9f968 is the
// last one rendered).
// decomp67: the mesh instance (vfx_mesh_instance__* in sum.symbols); the +0x14 / +0x18 / +0x1c fields and
// v20 / m2c (world, set by get_node_world_transform), v50 / m5c (local, get_node_transform) from the vfx object.
struct fx_rec98 {
    void init(vfx_node_desc *desc);   // 0x562b20 vfx_mesh_instance__init (another unit; decomp67)
    // (descriptor, frame, position, orientation).
    void render(vfx_node_desc *desc, float t, const vector3 *pos, const matrix *orient);   // 0x562ba0
    vector3 v00;                      // +0x00
    char _unk0c[0x14 - 0x0c];
    float alpha;                      // +0x14 the instance's alpha byte (vfx_instance::render; decomp67)
    character_instance *ci;           // +0x18 bound model (bind_node; decomp67)
    unsigned int tag : 28;            // +0x1c the model's mesh tag (decomp67; was _bits1c)
    unsigned int level : 1;           //       bit 28: vfx_instance_create's flag argument
    unsigned int bound : 1;           //       bit 29 (decomp67)
    unsigned int placed : 1;          //       bit 30 set by get_node_world_transform (decomp67)
    unsigned int hidden : 1;          //       bit 31 (get_tag_transform; decomp67)
    vector3 v20;                      // +0x20
    matrix m2c;                       // +0x2c
    vector3 v50;                      // +0x50
    matrix m5c;                       // +0x5c
    vector3 v80;                      // +0x80
    int glow;                         // +0x8c lamp_glow_add index (fx_create_instance; decomp64)
    fx_rec98 *next;                   // +0x90
    fx_rec98 *prev;                   // +0x94
};
// decomp67: the dummy state of an instance (vfx_dummy_state__*; the base's dummies at +0x30).
// decomp66: written in src/decomp/vsdk/unnamed_vfx_dummy_0x55fc70.cpp. The quaternion's empty constructor
// folds with vector3()'s, so 0x51df60 is unchanged.
struct fx_rec2c {
    void reset();                     // 0x55fc70 vfx_dummy_state__reset (the three flag bits, v08 0, q14 identity)
    // (dummy, t): position / rotation at frame t unless placed; through the model's bone when bound (ret 8).
    void update(vfx_dummy_desc *desc, float t);   // 0x55fc90 vfx_dummy_state__update
    character_instance *ci;           // +0x00 bound model (decomp67)
    unsigned int tag : 29;            // +0x04 (decomp67) the bone (compute_bone_matrix; decomp66)
    unsigned int bound : 1;           //       bit 29
    unsigned int placed : 1;          //       bit 30 (decomp66: not sampled)
    unsigned int hidden : 1;          //       bit 31
    vector3 v08;                      // +0x08 position
    quaternion q14;                   // +0x14 orientation (decomp67; was vector3 v14 and _unk20)
    fx_rec2c *next;                   // +0x24
    fx_rec2c *prev;                   // +0x28
};
// A particle of an emitter (0x28; 6000 at 0x2c42690 on fx_rec28_list).
// decomp66 (the emitter unit): the age and lifetime are seconds in 8.8, the angle 4.12 radians; +0x08 is
// the velocity and +0x14 the position (the effect frame relative to the emitter's origin). The accessors
// are inline (their COMDATs: 0x5622f0-0x562420 in the emitter object, 0x564c60 / 0x564c70 in the mesh
// object). sum.symbols has the position and velocity accessors the other way round (D4607).
struct fx_rec28 {
    float get_age() { return fixed8_to_float(life.v); }            // 0x564c60 vfx_particle__get_age
    float get_lifetime() { return fixed8_to_float(lifetime); }     // 0x564c70 vfx_particle__get_lifetime
    void set_lifetime(float s) { lifetime = (ushort)float_to_fixed8(s); }   // 0x5622f0
    float get_size() { return fixed12_to_float(angle); }           // 0x562540 vfx_particle__get_size (the angle)
    void set_size(float a) { angle = (short)float_to_fixed12(a); } // 0x562310 vfx_particle__set_size
    void set_pos(const vector3 &v) { pos = v; }                    // 0x562340 (sum.symbols vfx_particle__set_velocity)
    void set_vel(const vector3 &v) { vel = v; }                    // 0x562360 (sum.symbols vfx_particle__set_pos)
    vector3 get_pos() { return pos; }                              // 0x562400 (sum.symbols vfx_particle__get_velocity)
    vector3 get_vel() { return vel; }                              // 0x562420 (sum.symbols vfx_particle__get_pos)
    fixed8 life;                      // +0x00 age; -1.0 free (vfx_instance_create, the emitter update)
    ushort lifetime;                  // +0x02 8.8 (compared unsigned with life)
    short angle;                      // +0x04 4.12 (sum.symbols: size)
    short _pad06;
    vector3 vel;                      // +0x08 (was v08)
    vector3 pos;                      // +0x14 (was v14)
    fx_rec28 *next;                   // +0x20
    fx_rec28 *prev;                   // +0x24
};
// decomp66: the state of one force of an instance (the record vfx_force_state__update /
// vfx_force__get_transform run on; the vfx channel unit). The two rotations are quaternions (their
// empty constructor folds with vector3()'s, so the implicit constructor 0x51df80 is unchanged); the
// old v0c / v18 / v28 / v34 had no reader.
struct fx_rec4c {
    void clear();                     // 0x55b440 vfx_node_state__clear (time 0, the three flag bits cleared)
    // (force, t): the force's position / rotation at time t (the previous ones kept), through the owner's
    // bone when on_bone (ret 8).
    void update(vfx_force_desc *f, float t);   // 0x55b460 vfx_force_state__update
    // (force, out, t): position and rotation blended from the previous to the current state by t's
    // fraction, the force channels blended between frames (ret 0xc; sum.symbols names it
    // vfx_force__get_transform, but this is the state record).
    void get_transform(vfx_force_desc *f, vfx_force_params *out, float t);   // 0x55b730
    character_instance *owner;        // +0x00
    unsigned int bone : 29;           // +0x04
    unsigned int fixed : 1;           //       bit 29: not sampled
    unsigned int on_bone : 1;         //       bit 30: through the owner's bone matrix
    unsigned int _bit31 : 1;
    float time;                       // +0x08 the last time sampled (clamped to the last frame)
    vector3 pos;                      // +0x0c
    quaternion rot;                   // +0x18
    vector3 prev_pos;                 // +0x28
    quaternion prev_rot;              // +0x34
    fx_rec4c *next;                   // +0x44
    fx_rec4c *prev;                   // +0x48
};
// An emitter of an instance (0xb0; 128 at 0x27229d8 on fx_recb0_list).
// decomp67: vfx_emitter_instance__* in sum.symbols; +0x0c..+0x18 and +0x70 / +0x7c from the vfx object.
// decomp66: written in src/decomp/vsdk/unnamed_vfx_emitter_0x55ff00.cpp (times in frames, 15 a second;
// dt in seconds). The inline members' copies are COMDATs at the end of that object.
struct fx_recb0 {
    void init(vfx_emitter_desc *desc);   // 0x561220 vfx_emitter_instance__init (another unit; decomp67)
    // (descriptor, frame, position, orientation, wrapped this frame).
    void update(vfx_emitter_desc *desc, float t, const vector3 *pos, const matrix *orient, bool wrapped);   // 0x561670
    void render(vfx_emitter_desc *desc, float t);   // 0x561ec0 vfx_emitter_instance__render
    // decomp66: (desc, t, dt, emitter velocity): new particles for the frames since last_t (ret 0x10).
    void spawn(vfx_emitter_desc *desc, float t, float dt, const vector3 &vel);   // 0x55ff30
    // (desc, particle, fraction of the step, age, emitter velocity) (ret 0x14).
    void init_particle(vfx_emitter_desc *desc, fx_rec28 *p, float frac, float age, const vector3 &vel);   // 0x5601f0
    // (desc, particle, forces, count, dt): one integration step (ret 0x14; sum.symbols vfx_particle_apply_forces).
    void apply_forces(vfx_emitter_desc *desc, fx_rec28 *p, vfx_force_params *forces, int count, float dt);   // 0x5605f0
    // (desc, t, dt): the forces at each frame since last_t, then steps of at most 1/30 s (ret 0xc).
    void move_particles(vfx_emitter_desc *desc, float t, float dt);   // 0x560a00
    // (desc, t, wrapped): offset, rotation, the five shape values and the alpha at t (ret 0xc).
    void get_transform(vfx_emitter_desc *desc, float t, bool wrapped);   // 0x5612b0
    void render_d3d(vfx_emitter_desc *desc, float t);   // 0x561900 vfx_emitter_instance__render_d3d (no caller)
    // Inline (COMDATs 0x562380-0x5623b0, 0x562830): the shape values as floats.
    float get_fixed12_8c() { return fixed12_to_float(width); }      // 0x562380
    float get_fixed12_8e() { return fixed12_to_float(depth); }      // 0x562390
    float get_fixed12_90() { return fixed12_to_float(size); }       // 0x562830
    float get_fixed12_92() { return fixed12_to_float(speed); }      // 0x5623a0
    float get_fixed12_94() { return fixed12_to_float(speed_var); }  // 0x5623b0
    // Inline, defined in the emitter unit (COMDATs; thiscall, this unused by get_bitmaps / get_color).
    vfx_material *get_bitmaps(vfx_emitter_desc *desc, fx_rec28 *p, int *bitmaps);   // 0x562550 vfx_particle_get_bitmaps
    void get_color(vfx_emitter_desc *desc, vfx_material *mat, float t, ubyte *r, ubyte *g, ubyte *b);   // 0x562600
    void get_fade(vfx_emitter_desc *desc, float age, float life, float *size, float *fade, float *alpha);   // 0x5626b0
    void get_billboard_axes(vfx_emitter_desc *desc, fx_rec28 *p, vector3 *a, vector3 *b, vector3 *c,
                            float size, float fade);   // 0x562840 vfx_particle_get_billboard_axes
    fx_rec4c *links[3];               // +0x00 the instance's fx_rec4c records the emitter's links name
    union {
        unsigned int flags;           // +0x0c bit 0: stopped (vfx_instance::stop; decomp67)
        struct {
            unsigned int stopped : 1; //       decomp66: the same bit 0
            unsigned int _flags1 : 23;
            unsigned int alpha : 8;   //       decomp66: the material's curve 48 * 255 (get_transform)
        };
    };
    unsigned int tag : 28;            // +0x10 (decomp67)
    unsigned int bound : 1;           //       bit 28
    unsigned int placed : 1;          //       bit 29
    unsigned int hidden : 1;          //       bit 30
    unsigned int _bit31 : 1;
    character_instance *ci;           // +0x14
    float time;                       // +0x18 decomp66: seconds since the first update (was _unk18)
    vector3 v1c;                      // +0x1c decomp66: the instance position (update's pos)
    matrix m28;                       // +0x28 decomp66: the instance orientation
    vector3 v4c, v58, v64, v70;       // +0x4c..+0x70 (v70: the position get_node_world_transform sets)
                                      //       decomp66: v4c / v58 the emission point before / after this
                                      //       update, v64 the origin (offset at frame 0 plus the first
                                      //       position), v70 the offset at t
    quaternion q7c;                   // +0x7c orientation (decomp67; was vector3 v7c)
    short width;                      // +0x8c decomp66: 4.12 at t (ch_a0); was _unk8c
    short depth;                      // +0x8e ch_ac
    short size;                       // +0x90 ch_b8
    short speed;                      // +0x92 ch_c4
    short speed_var;                  // +0x94 ch_d0
    ushort num_live;                  // +0x96 live particles (fx_missile_hit waits for none; decomp64)
    char _unk98[0x9c - 0x98];
    fx_rec28 *particles;              // +0x9c
    float last_t;                     // +0xa0 decomp66: the frame of the last update
    float spawn_accum;                // +0xa4 particles owed * 320
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
// decomp53 (effect streaming): the base's resident bit; true when the name is not a base.
bool effects_base_is_resident(const char *name);   // 0x51de90
void effect_base_set_info(unsigned int index, ushort num_nodes, int table_size, int size, ushort version);
void effect_base_read(unsigned int index, int *out_nodes, int *out_table_size, int *out_size, int *out_version);
vfx_base *vfx_preload(const char *name, int *out_index);
int effects_parse_arr_line(File *f, int *line, char *name, int *category, unsigned int *num,
                           char names[][ARR_NAME_LEN]);
void effects_reset_level();
void effects_init_pools();
int effects_load_bases();
void effects_show_memory_usage();

// decomp60: the rest of the unit (0x51d250-0x51df0f).
int effects_hash_name(const char *name);                // 0x51db50
// Returns 0, or -1 when preload.rfx does not load (D4005; level_preload_effects tail-jumps to it, decomp29).
int effects_load_preload_rfx();                         // 0x51dbb0

void effects_release_level_memory();  // 0x51dc20 (level_unload; decomp42)
void effects_release_range(char *start, int size);      // 0x51dc50 releases the resident bases whose data is inside
int  effects_stub_ret1();                               // 0x51dca0 (no caller)
int  vfx_all_resident(const char **names);              // 0x51dcb0 25 names (no caller)
bool effect_is_cutscene_base(int name_index);           // 0x51dec0 (fx_pause_for_cutscene / fx_resume_after_cutscene)
// decomp64: defined in effect_mem.cpp; fx_frame clears it every frame (0x48a242).
extern fx_rec98 *vfx_last_mesh_instance;                // 0x2c9f968
void effects_release_cutscene_memory();   // 0x51def0 (cutscene_cleanup; decomp52)
// (base, a, flag): a new instance with the base's records, NULL when no record is free.
vfx_instance *vfx_instance_create(vfx_base *base, int a, bool flag);   // 0x51d250
// decomp48 (the interface manager's model lights "inv-lights1.vfx"): an instance of an effect file and
// its release. vfx_load's second argument is 1 at that call (meaning not known).
void vfx_instance_free(vfx_instance *vi);                // 0x51d7b0
vfx_instance *vfx_load(const char *name, int a);        // 0x51dd00

// decomp67: the vfx object's inline members (vfx.h). Out-of-line COMDATs at the end of that object
// (0x538590-0x538647), which is built without inline expansion; no other unit calls them.
inline vfx_base *vfx_instance::get_base()               // 0x538590 vfx_instance__get_base
{ return effect_get_base(id_bits >> 9); }
inline fx_rec98 *vfx_instance::list18_item(int n)       // 0x5385b0
{ fx_rec98 *p = nodes; while (p != NULL && n > 0) { p = p->next; n--; } return p; }
// Steps n by 2 (sic): light tag n finds light (n + 1) / 2 (NOTES decomp67, game bug).
inline vfx_light_desc *vfx_instance::list1c_item(int n) // 0x5385d0
{ vfx_light_desc *p = lights; while (p != NULL && n > 0) { p = p->next; n -= 2; } return p; }
inline fx_recb0 *vfx_instance::list20_item(int n)       // 0x5385f0
{ fx_recb0 *p = emitters; while (p != NULL && n > 0) { p = p->next; n--; } return p; }
inline vfx_inst21c *vfx_instance::list24_item(int n)    // 0x538610
{ vfx_inst21c *p = insts21c; while (p != NULL && n > 0) { p = p->next; n--; } return p; }
inline fx_rec2c *vfx_instance::list28_item(int n)       // 0x538630
{ fx_rec2c *p = rec2c_list; while (p != NULL && n > 0) { p = p->next; n--; } return p; }

#endif
