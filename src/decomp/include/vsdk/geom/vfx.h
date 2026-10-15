// vsdk/geom/vfx.h - effect bases as the effect code sees them (vsdk\geom in sum.symbols, 0x536bc0-0x536e50
// and ptr_relocate 0x564d50; file names not known). Not original source.
// decomp60: the instance record (vfx_instance = effect_mem.h's fx_rec48, D4000), the instance light
// (vfx_light_desc = fx_rec30) with its file track (vfx_light_track, D4001), the key channels, and the
// attached-light unit (0x51fb10-0x520413, src/decomp/vsdk/unnamed_attached_lights_0x51fb10.cpp).
#ifndef VSDK_GEOM_VFX_H
#define VSDK_GEOM_VFX_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"                 // vfx_light_desc::pos (decomp48)
#include "vsdk/math/matrix.h"                 // attached_light::orient (quaternion)
#include "vsdk/math/fixed.h"

class character_instance;
struct fx_rec98;                              // effect_mem.h (the node / mesh-instance records)
struct fx_rec2c;
struct fx_rec4c;
struct fx_recb0;

void ptr_relocate(void *p, char *base);       // 0x564d50 (*p += base unless NULL)

// One emitter record of a base (0xe8 bytes, the array at vfx_base +0x28; name ours). Only what
// vfx_instance_create reads.
class vfx_emitter_desc {
public:
    // (rate + 1) * frames / 4800 (thiscall; a COMDAT after effect_mem.cpp's record constructors).
    float get_length_seconds()                // 0x51e020 vfx_get_length_seconds
    { return (rate + 1.0f) * frames * (1.0f / 4800.0f); }
    char _unk00[0x0c];
    char *links[3];                           // +0x0c records of the base's 0xa0-byte table (+0x34), or NULL
    unsigned int num_links : 31;              // +0x18
    unsigned int _bit31 : 1;
    short num_particles;                      // +0x1c particle records (fx_rec28) per instance
    short _pad1e;
    int frames;                               // +0x20
    float rate;                               // +0x24
    char _unk28[0xe8 - 0x28];
};
LAYOUT_CHECK(vfx_emitter_desc, sizeof(vfx_emitter_desc) == 0xe8);

// The loaded effect base ("vfx base") an effect id resolves to. Only +0x1c is written by effect_mem.cpp.
// decomp60: the record counts and fields vfx_instance_create / vfx_load read (names ours).
class vfx_base {
public:
    void load_bitmaps(int flag);              // 0x536e50 vfx_base__load_bitmaps
    void relocate_node(char *a, char *b);     // 0x536bc0 vfx_node_relocate (argument meaning unsure)
    char _unk00[0x04];
    short num_nodes;                          // +0x04 fx_rec98 records per instance
    short num_lights;                         // +0x06 vfx_light_desc records per instance (light_tracks)
    short num_emitters;                       // +0x08 fx_recb0 records (emitters)
    short num_rec2c;                          // +0x0a fx_rec2c records
    short num_rec4c;                          // +0x0c fx_rec4c records (the 0xa0-byte table at +0x34)
    char _unk0e[0x04];
    short num_frames;                         // +0x12 (cast_effect_track_add: frames / 15 * 1.5 s; decomp39)
    char _unk14[0x04];
    short flags;                              // +0x18 bits 1 and 2 are copied into the instance's bits 7 / 8
                                              //       (vfx_load; meaning not known)
    short _pad1a;
    int category;                             // +0x1c copied from the base record's category
    int _unk20;                               // +0x20
    char *light_tracks;                       // +0x24 0x70 bytes per light of an instance (ui_3d_begin; decomp48)
    vfx_emitter_desc *emitters;               // +0x28 num_emitters records
    char _unk2c[0x08];
    char *rec4c_table;                        // +0x34 0xa0 bytes per fx_rec4c record
    char _unk38[0x10];
    vector3 pos;                              // +0x48 copied into the instance by vfx_load
    float _unk54;                             // +0x54 copied to the instance's +0x3c
};

// ---- key channels of the effect files (vsdk\geom; names from sum.symbols, layout from 0x55bc00) ----
// A run-length coded int16 key channel (0xc bytes). eval is out of line in another vsdk\geom unit;
// relocate is inline (its COMDAT 0x5202b0 is in the attached-light unit).
class vfx_channel {
public:
    int eval(int frame);                      // 0x55bc00 vfx_channel__eval (int16 value, AX)
    void relocate(char *base)                 // 0x5202b0 vfx_track__relocate (thiscall, ret 4)
    { frame = 0; cursor = 0; ptr_relocate(this, base); }
    char *data;                               // +0x00
    short size;                               // +0x04
    short base_value;                         // +0x06
    short cursor;                             // +0x08 cached key, reset by relocate
    ushort frame : 15;                        // +0x0a cached frame, reset by relocate
    ushort flag : 1;
};
LAYOUT_CHECK(vfx_channel, sizeof(vfx_channel) == 0xc);

// Three channels: x +0, y +0xc, z +0x18 (0x24 bytes).
class vfx_vec_channel {
public:
    void relocate(char *base);                // 0x55bb50 vfx_vec_channel__relocate (another unit)
    // The position at a frame (hidden result, ret 8; COMDAT 0x520310). The z channel is evaluated
    // first (the constructor's arguments are pushed right to left).
    vector3 sample(int frame)                 // 0x520310 vfx_vector_track__sample
    { return vector3(fixed4_to_float((short)x.eval(frame)), fixed4_to_float((short)y.eval(frame)),
                     fixed4_to_float((short)z.eval(frame))); }
    vfx_channel x, y, z;
};
LAYOUT_CHECK(vfx_vec_channel, sizeof(vfx_vec_channel) == 0x24);

// A per-frame bit array (0xc bytes at the light track's +0x64). get is out of line elsewhere; the
// inline relocate is the COMDAT 0x5202d0 (sum.symbols: vfx_ptr__relocate).
class BitArray {
public:
    int get(int bit);                         // 0x55be80 BitArray__get (1 when set)
    void relocate(char *base)                 // 0x5202d0 vfx_ptr__relocate (thiscall, ret 4)
    { ptr_relocate(this, base); }
    char *data;                               // +0x00
    int _unk04;
    int _unk08;
};
LAYOUT_CHECK(BitArray, sizeof(BitArray) == 0xc);

// The header of an effect file record (0x10 bytes): two name / link fields, a pointer and the frame count.
class vfx_node_header {
public:
    void relocate(char *header_base, char *base);   // 0x55bbd0 vfx_node_header__relocate (another unit)
    int name;                                 // +0x00
    int link;                                 // +0x04
    char *data;                               // +0x08
    ushort _bit0 : 1;                         // +0x0c
    ushort num_frames : 15;                   //       frames of the light's tracks (sample: SHR 1)
    short _unk0e;
};
LAYOUT_CHECK(vfx_node_header, sizeof(vfx_node_header) == 0x10);

// The file track of one effect light (0x70 bytes, vfx_base::light_tracks + 0x70 * index; class name
// ours, its methods are vfx_light_desc__* in sum.symbols, D4001). relocate is out of line in the
// attached-light unit, the samplers are inline (COMDATs 0x5202f0, 0x520380-0x5203b0 there).
class vfx_light_track {
public:
    void relocate(char *header_base, char *base);     // 0x51fb40 vfx_light_desc__relocate
    vector3 get_position(int frame)           // 0x5202f0 vfx_light_desc__get_position (hidden result, ret 8)
    { return pos.sample(frame); }
    short sample_range(int frame)             // 0x520380 vfx_light_desc__sample_34
    { return (short)range.eval(frame); }
    ubyte sample_r(int frame)                 // 0x520390 vfx_light_desc__sample_40
    { return (ubyte)r.eval(frame); }
    ubyte sample_g(int frame)                 // 0x5203a0 vfx_light_desc__sample_4c
    { return (ubyte)g.eval(frame); }
    ubyte sample_b(int frame)                 // 0x5203b0 vfx_light_desc__sample_58
    { return (ubyte)b.eval(frame); }
    vfx_node_header header;                   // +0x00
    vfx_vec_channel pos;                      // +0x10 position, 1/16 units
    vfx_channel range;                        // +0x34 range, 8.8 fixed
    vfx_channel r, g, b;                      // +0x40 / +0x4c / +0x58 colour 0..255
    BitArray on;                              // +0x64 on / off per frame
};
LAYOUT_CHECK(vfx_light_track, sizeof(vfx_light_track) == 0x70);

// One light of a vfx instance (decomp48, ui_3d_begin 0x412400; names ours). An instance's lights form a
// circular list from vfx_instance::lights through next / prev (effect_mem.h's fx_rec30, 0x30 bytes,
// the pool effects_init_pools threads; its implicit constructor is 0x51df60, D4000).
// sum.symbols names the members vfx_node_* (create / free / update / get_scale) and vfx_light_desc__sample.
class vfx_light_desc {
public:
    // Samples the light's tracks (position, size, colour) at time t. track = the base's light_tracks +
    // 0x70 * light index (thiscall, ret 8; the body reads track +0xc as a frame count).
    void sample(void *track, float t);        // 0x51fdc0 vfx_light_desc__sample
    void create_light(vfx_light_track *track, bool no_light);   // 0x51fb90 vfx_node_create_light
    void free_light();                        // 0x51fc80 vfx_node_free_light
    void update_light(void *track, float t, const vector3 *offset, const matrix *orient);   // 0x51fcb0 vfx_node_update_light
    float get_scale()                         // 0x5203f0 vfx_node__get_scale (inline; the range in units)
    { return fixed8_to_float(range); }
    character_instance *owner;                // +0x00 the model the light follows (bone)
    unsigned int bone : 23;                   // +0x04 bone index
    int slot : 6;                             //       attached_light slot, -1 none
    unsigned int on_bone : 1;                 //       bit 29: transformed by the owner's bone
    unsigned int fixed : 1;                   //       bit 30: position not sampled
    unsigned int on : 1;                      //       bit 31: the track's on / off bit
    vector3 pos;                              // +0x08 offset from the instance's origin
    vector3 offset;                           // +0x14 added before the light is placed
    ubyte r, g, b;                            // +0x20 colour, 0..255
    ubyte _pad23;
    short range;                              // +0x24 range * 256
    short _pad26;
    vfx_light_desc *next;                     // +0x28
    vfx_light_desc *prev;                     // +0x2c
};
LAYOUT_CHECK(vfx_light_desc, sizeof(vfx_light_desc) == 0x30);

// A vfx instance (the effect object +0xa0; 0x48 bytes, 48 records at 0x2c7d010 threaded on
// vfx_instance_free_list; effect_mem.h's fx_rec48, D4000). Members spells.cpp calls (decomp23); argument
// order from the call sites (sum.symbols lists the node index first for get_node_transform).
class vfx_instance {
public:
    void process(int a, const vector3 *pos, const matrix *orient);   // 0x536fb0 vfx_instance_process (callers pass 0)
    void get_node_transform(vector3 *pos, matrix *orient, int node); // 0x537840
    // The world transform of a node, the vector and matrix passed by value (spell_probe_ground_point).
    void get_node_world_transform(int node, vector3 pos, matrix orient, int a, int b, int c); // 0x537a10
    int  find_tag(const char *name);                                 // 0x537f10, -1 (">>> FAILED TO GET TAG %s")
    int  get_tag_transform(int tag, int a, int b);                   // 0x537f70 (result unused by spells.cpp)
    float get_time_left();                                           // 0x538510 (decomp37)
    // decomp60 (vfx_instance_create / vfx_instance_free; both in another vsdk\geom unit).
    void init_nodes(vfx_base *base, int a, bool level);             // 0x536a70 vfx_instance_init_nodes
    void release_nodes();                                            // 0x5369f0 vfx_instance_release_nodes
    unsigned int id_bits;                                            // +0x00 effect name index << 9 (cast_effect_track_add; decomp39);
                                                                     // bits 0-8 flags: 0x40 in use, 0x80 / 0x100 the base's flag_a / flag_b,
                                                                     // interface_frame clears bit 2 of the UI lights' instance (decomp48)
    int _unk04;                                                      // +0x04
    int category;                                                    // +0x08 the base record's category (vfx_load)
    int _unk0c, _unk10;
    float length;                                                    // +0x14 longest emitter, seconds
    fx_rec98 *nodes;                                                 // +0x18 list heads, NULL empty
    vfx_light_desc *lights;                                          // +0x1c first light, NULL none (ui_3d_begin; decomp48)
    fx_recb0 *emitters;                                              // +0x20
    int _unk24;
    fx_rec2c *rec2c_list;                                            // +0x28
    fx_rec4c *rec4c_list;                                            // +0x2c
    vector3 pos;                                                     // +0x30 the base's pos (vfx_load)
    float _unk3c;                                                    // +0x3c the base's +0x54
    vfx_instance *next;                                              // +0x40
    vfx_instance *prev;                                              // +0x44
};
LAYOUT_CHECK(vfx_instance, sizeof(vfx_instance) == 0x48);

// ---- attached lights (0x51fb10-0x520413, src/decomp/vsdk/unnamed_attached_lights_0x51fb10.cpp) ----
// A light attached to an object (0x98 bytes; 24 slots at 0x2cb45d0, attached_light_create / _free). The
// constructor is inline (decomp28, D805): levelrender.cpp's array initialiser loops over reset() with
// no constructor call, while init_attached_lights calls the out-of-line copy attached_light__ctor
// (0x520280, which builds the quaternion and the two vectors first). decomp60: the layout (names ours);
// levelrender_sum_point_lights reads the same fields through gr_light (D4002).
#define MAX_ATTACHED_LIGHTS 24
class attached_light {
public:
    attached_light() { reset(); }             // 0x520280 attached_light__ctor (COMDAT)
    void reset();                             // 0x520210 attached_light__reset (identity orientation, defaults)
    ubyte type;                               // +0x00 -1 free slot, 0 point (attached_light_create), 1 / 4 (create_b)
    ubyte _pad01[3];
    quaternion orient;                        // +0x04 identity
    vector3 pos;                              // +0x14
    vector3 world_pos;                        // +0x20 levelrender's light position
    float r, g, b;                            // +0x2c
    float intensity;                          // +0x38 1.0
    float angle_a;                            // +0x3c attached_light_create_b
    float angle_b;                            // +0x40
    float _unk44;                             // +0x44
    bool has_range;                           // +0x48
    ubyte _pad49[3];
    float range_near;                         // +0x4c
    float range_far;                          // +0x50 40.0 after reset
    bool _unk54;                              // +0x54
    ubyte _unk55[0x98 - 0x55];
};
LAYOUT_CHECK(attached_light, sizeof(attached_light) == 0x98);
extern attached_light attached_light_slots[MAX_ATTACHED_LIGHTS];   // 0x2cb45d0

// (pos, intensity, range near, range far, r, g, b): a point light (type 0) in a free slot, -1 when all 24
// are used. living_entity_init pushes (pos, 1.0, 0, attenuation, r, g, b) (decomp37; D4003).
int  attached_light_create(const vector3 *pos, float intensity, float range_near, float range_far,
                           float r, float g, float b);              // 0x520160
// (pos, intensity, angle a, angle b, range near, range far, r, g, b, kind): type 1 when kind, else 4.
int  attached_light_create_b(const vector3 *pos, float intensity, float angle_a, float angle_b,
                             float range_near, float range_far, float r, float g, float b, bool kind);   // 0x5200b0
void attached_light_free(int index);                 // 0x5201f0
void attached_lights_init_slots();                   // 0x520070 every slot free
void attached_lights_clear_slots();                  // 0x520090 (the same loop)
// Inline: its only copy is a COMDAT among the attached-light unit's (0x5203c0, between the track
// samplers and vfx_node__get_scale). (pos, intensity, range far, r, g, b), range near 0.
inline int attached_light_create_from_vfx(const vector3 *pos, float intensity, float range_far,
                                          float r, float g, float b)   // 0x5203c0
{ return attached_light_create(pos, intensity, 0, range_far, r, g, b); }

// The camera shake of the running effects (state_game_frame adds it to the view; decomp42).
void vfx_get_camera(vector3 *pos, quaternion *rot);   // 0x5383e0

#endif
