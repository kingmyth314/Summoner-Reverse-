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
struct vfx_node_ref;                          // below

void ptr_relocate(void *p, char *base);       // 0x564d50 (*p += base unless NULL)
// decomp67: (ptr*, base): 0 stays, -1 becomes base, else base is added (the record names; another unit).
void reloc_ptr(void *p, char *base);          // 0x533d90

// decomp67 (the vfx object 0x536950-0x538630, src/decomp/vsdk/unnamed_vfx_0x536950.cpp): every record kind
// of a base starts with its name (+0x00) and its parent's name (+0x04), relocated by reloc_ptr against the
// header base; vfx_instance's tag functions read them (get_tag_name +0, get_node_name +4). The tag kinds
// of vfx_base::decode_tag (names ours); vfx_node_ref::kind uses the same numbers (0 node, 4 emitter).
#define VFX_TAG_MESH          0               // node_descs / vfx_instance::nodes
#define VFX_TAG_EMITTER_PART  1               // a sub-tag of an emitter (count: the emitter's num_particles)
#define VFX_TAG_LIGHT         2               // light_tracks / lights
#define VFX_TAG_DUMMY         3               // dummies / rec2c_list
#define VFX_TAG_EMITTER       4               // emitters / vfx_instance::emitters
#define VFX_TAG_VIF           5               // descs21c / insts21c

// One emitter record of a base (0xe8 bytes, the array at vfx_base +0x28; name ours). Only what
// vfx_instance_create reads.
class vfx_emitter_desc {
public:
    // (rate + 1) * frames / 4800 (thiscall; a COMDAT after effect_mem.cpp's record constructors).
    float get_length_seconds()                // 0x51e020 vfx_get_length_seconds
    { return (rate + 1.0f) * frames * (1.0f / 4800.0f); }
    void relocate(char *header_base, char *base);   // 0x561190 vfx_emitter__relocate (another unit; decomp67)
    const char *name;                         // +0x00 (decomp67; was char _unk00[0x0c])
    const char *parent;                       // +0x04
    char _unk08[0x04];
    char *links[3];                           // +0x0c records of the base's 0xa0-byte table (+0x34), or NULL
    unsigned int num_links : 31;              // +0x18
    unsigned int _bit31 : 1;                  //       decomp67: the emitter can be bound to a mesh tag
    // decomp67 (D4702): a signed 16-bit bit field (shl / sar 16 in vfx_instance_create, decode_tag and
    // get_num_tags; was short num_particles, short _pad1e); also the emitter's sub-tag count.
    int num_particles : 16;                   // +0x1c particle records (fx_rec28) per instance
    int _bits1c_16 : 16;
    int frames;                               // +0x20
    float rate;                               // +0x24
    // decomp63 (levelrender_add_effects / levelrender_effect_apply_vis; names ours).
    unsigned int vis_flags;                   // +0x28 0x400000 shown by a $vis tag, 0x800000 always shown
    char _unk2c[0x3c - 0x2c];
    vector3 centre;                           // +0x3c bounding sphere (effect frame)
    float radius;                             // +0x48
    char _unk4c[0xe8 - 0x4c];
};
LAYOUT_CHECK(vfx_emitter_desc, sizeof(vfx_emitter_desc) == 0xe8);

// decomp64 (the fx unit): the node descriptors (vfx_base +0x20, 0xa8 bytes each) and the +0x2c records (0x21c)
// as fx_create_instance and fx_frame read them. Names ours.
// decomp67: the node descriptors are the meshes (vfx_mesh__* in sum.symbols).
struct vfx_node_desc {
    void relocate(char *header_base, char *base);   // 0x5645b0 vfx_mesh__relocate (another unit; decomp67)
    void load_bitmap(int flags);              // 0x564720 vfx_mesh__load_bitmap (decomp67)
    const char *name;                         // +0x00 (decomp67; was ubyte _unk00[0x18])
    const char *parent;                       // +0x04
    ubyte _unk08[0x04];
    ubyte bind_flags;                         // +0x0c bit 0: can be bound to a mesh tag (decomp67)
    ubyte _unk0d[0x0b];
    // decomp63 (D4309): levelrender_effect_apply_vis rewrites the low word only (0x483ff0-0x484003):
    // 0x200 shown by a $vis tag, 0x400 always shown (0x600 = drawn).
    union {
        int flags;                            // +0x18 0x40: a lamp glow (fx_create_instance)
        ushort vis_flags;
    };
    vector3 pos;                              // +0x1c decomp63: the node's position in the effect frame
    float glow_radius;                        // +0x28 (decomp63: also the sort radius of its draw entry)
    float cull_radius;                        // +0x2c decomp63: gr_cull_sphere radius (levelrender_add_effect_entry)
    ubyte _unk30[0x18];
    vector3 glow_pos;                         // +0x48 decomp63: lamp_glow_update_effects' ray origin (effect frame)
    ubyte _unk54[0xa8 - 0x54];
};
LAYOUT_CHECK(vfx_node_desc, sizeof(vfx_node_desc) == 0xa8);
// decomp67: the vif emitter descriptors (vif_desc__* in sum.symbols).
struct vfx_desc21c {
    void relocate(char *header_base);         // 0x533be0 vif_desc__relocate (another unit; decomp67)
    void load_bitmap(int flag);               // 0x533c20 vif_desc__load_bitmap
    const char *name;                         // +0x00 (decomp67; was ubyte _unk00[0x4c])
    const char *parent;                       // +0x04
    vector3 pos;                              // +0x08 (get_node_world_transform measures from it)
    ubyte _unk14[0x4c - 0x14];
    float time_a;                             // +0x4c
    float time_b;                             // +0x50 (time_a + time_b) s (fx_frame kind 16)
    ubyte _unk54[0x6a - 0x54];
    ushort vis_flags;                         // +0x6a decomp63: 0x80 always shown, 0x100 shown by a $vis tag
                                              //       (0x180 = drawn; the vif_* particle functions take these)
                                              //       decomp67: 0x200 can be bound to a mesh tag
    ubyte _unk6c[0x21c - 0x6c];               // +0x6c the bitmap, +0x70 its name
};
LAYOUT_CHECK(vfx_desc21c, sizeof(vfx_desc21c) == 0x21c);

// decomp63 (the render-scene unit 0x481680): the base's tags (+0x3c) and the per-instance records of the
// +0x2c descriptors (a list from vfx_instance +0x24), as levelrender_effect_apply_vis /
// levelrender_add_effects / fx_draw_flagged_particles read them. Names ours.
struct vfx_tag_ref {                          // 8 bytes
    int kind;                                 // +0 0 node, 1 emitter, 2 +0x2c record
    void *rec;                                // +4 the vfx_node_desc / vfx_emitter_desc / vfx_desc21c
};
struct vfx_base_tag {                         // 0x14 bytes ("$vis..." names)
    void relocate(char *header_base, char *base);   // 0x565400 vfx_group__relocate (another unit; decomp67)
    char *name;                               // +0x00
    ubyte _unk04[0x08];
    int count;                                // +0x0c
    vfx_tag_ref *refs;                        // +0x10
};
// decomp67: vif_instantiate_level makes them, vif_record_release frees them; the fields from the vfx object.
struct vfx_inst21c {                          // the instance record of a vfx_desc21c
    vector3 pos;                              // +0x00 (decomp67)
    matrix orient;                            // +0x0c
    ubyte _unk30[0x68 - 0x30];
    ushort flags;                             // +0x68 0x10 updated this frame (fx_draw_flagged_particles)
                                              //       decomp67: 2 placed by get_node_world_transform, 4 bound,
                                              //       8 hidden (get_tag_transform)
    short tag;                                // +0x6a the mesh tag it is bound to (decomp67)
    character_instance *ci;                   // +0x6c
    ubyte _unk70[0x02];
    ushort num_live;                          // +0x72 live particles (decomp67)
    ubyte _unk74[0xe0 - 0x74];
    vfx_inst21c *next;                        // +0xe0
    vfx_inst21c *prev;                        // +0xe4 (decomp67)
};
// decomp67: the vfx object's calls into the vif unit (cdecl).
vfx_inst21c *vif_instantiate_level(vfx_desc21c *desc, bool level);   // 0x5337a0
void vif_record_release(vfx_inst21c *em, vfx_desc21c *desc);       // 0x5339b0
// Inline: the out-of-line copy is 0x5385a0 (sum.symbols vfx_get_word_72), a COMDAT at the end of the vfx
// object (decomp67).
inline int vif_record_num_live(vfx_inst21c *em)   // 0x5385a0
{ return em->num_live; }

// decomp67: the other record kinds of a base (names ours; their methods are in other vsdk\geom units).
struct vfx_dummy_desc {                       // 0x64 bytes, vfx_base +0x30 (count +0x0a)
    void relocate(char *header_base, char *base);   // 0x55fed0 vfx_dummy__relocate
    const char *name;                         // +0x00
    const char *parent;                       // +0x04
    ubyte _unk08[0x04];
    ushort flags;                             // +0x0c 0x8000: can be bound
    ubyte _unk0e[0x64 - 0x0e];                // +0x10 position channel, +0x34 rotation channel
};
LAYOUT_CHECK(vfx_dummy_desc, sizeof(vfx_dummy_desc) == 0x64);
struct vfx_force_desc {                       // 0xa0 bytes, vfx_base +0x34 (count +0x0c)
    void relocate(char *header_base, char *base);   // 0x55b960 vfx_force__relocate
    ubyte _unk00[0xa0];
};
LAYOUT_CHECK(vfx_force_desc, sizeof(vfx_force_desc) == 0xa0);
struct vfx_camera_desc {                      // 0x64 bytes, vfx_base +0x38 (count +0x0e)
    void relocate(char *header_base, char *base);   // 0x5655c0 vfx_camera__relocate
    // false outside its frame range, else the camera at frame t.
    bool get_transform(float t, vector3 *pos, quaternion *rot);   // 0x565470 vfx_camera__get_transform
    ubyte _unk00[0x64];
};
LAYOUT_CHECK(vfx_camera_desc, sizeof(vfx_camera_desc) == 0x64);
struct vfx_tex_layer {                        // 0x14 bytes, two at vfx_material +0x0c
    int bitmap;                               // +0x00 -1 none
    ubyte _unk04[0x10];
};
struct vfx_material {                         // 0x4c bytes, vfx_base +0x44 (count +0x14)
    void relocate(char *header_base, char *base);   // 0x565280 vfx_material__relocate
    void load_bitmaps(int flags);             // 0x5652e0 vfx_material__load_bitmaps
    float eval_curve34(float t);              // 0x564ff0 0..1
    float eval_curve48(float t);              // 0x5651b0 0..1
    float eval_curve44(float t);              // 0x5650e0 0..1
    ushort flags;                             // +0x00 bits 3-5 blend mode, bit 6 (setup_render_state)
    ubyte _unk02[0x0a];
    vfx_tex_layer layers[2];                  // +0x0c
    ubyte _unk34[0x4c - 0x34];                // +0x34 curve pointers, the name +0x38, its bitmap +0x3c
};
LAYOUT_CHECK(vfx_material, sizeof(vfx_material) == 0x4c);
struct vfx_material_link {                    // what vfx_base +0x40 points to (setup_render_state)
    ubyte _unk00[0x0c];
    ubyte material;                           // +0x0c index into vfx_base::materials
};
// What vfx_instance::render_node draws (8 bytes; the effect draw entries carry one at +0x14).
struct vfx_node_ref {
    short index;                              // +0 node or emitter index
    ubyte kind;                               // +2 0 node, 4 emitter, 6 line trail (levelrender_add_effect_entry)
    ubyte _pad3;
    void *inst;                               // +4 the instance's fx_rec98 / fx_recb0
};
LAYOUT_CHECK(vfx_node_ref, sizeof(vfx_node_ref) == 8);

// The loaded effect base ("vfx base") an effect id resolves to. Only +0x1c is written by effect_mem.cpp.
// decomp60: the record counts and fields vfx_instance_create / vfx_load read (names ours).
class vfx_base {
public:
    // decomp67: sum.symbols calls these vfx_instance__ctor (0x538570) and vfx_instance__clear, but the
    // fields are vfx_base's (the counts +0x04..+0x16, flags +0x18, +0x40, pos +0x48; D4701). No caller:
    // bases are loaded from the effect files and relocated.
    vfx_base();                               // 0x538570 (constructs pos, then clear)
    void clear();                             // 0x5369c0 the counts except num_materials, the low 13 flag bits, +0x40
    void load_bitmaps(int flag);              // 0x536e50 vfx_base__load_bitmaps
    // decomp67: reloc_ptr of the name against header_base, ptr_relocate of the table pointers against
    // base, then every record; the record counts are capped (300 meshes, 15 lights, 90 emitters, 50
    // dummies, 100 forces). effect_mem.cpp passes (end of the table, base address).
    void relocate_node(char *header_base, char *base);   // 0x536bc0 vfx_node_relocate
    int  get_num_tags();                      // 0x538340 vfx_base__get_num_tags (+4 + +6 + +0xa + +0x16 + vif tags; decomp64)
    // decomp67 (the vfx object).
    void init_vis_flags();                    // 0x536ed0 vfx_base__init_vis_flags
    void decode_tag(int index, int *kind, int *list, int *sub);   // 0x5372a0 vfx_base__decode_tag (VFX_TAG_*)
    void page_in_particle_bitmaps();          // 0x538390 vfx_base__page_in_particle_bitmaps
    const char *name;                         // +0x00 the effect file name (fx_disable_by_effect_name; decomp64)
    short num_nodes;                          // +0x04 fx_rec98 records per instance
    short num_lights;                         // +0x06 vfx_light_desc records per instance (light_tracks)
    short num_emitters;                       // +0x08 fx_recb0 records (emitters)
    short num_rec2c;                          // +0x0a fx_rec2c records (decomp67: the dummies at +0x30)
    short num_rec4c;                          // +0x0c fx_rec4c records (the 0xa0-byte table at +0x34)
    short num_cameras;                        // +0x0e records at +0x38 (decomp67; was char _unk0e[2])
    short num_tags;                           // +0x10 decomp63 (levelrender_effect_apply_vis)
    short num_frames;                         // +0x12 (cast_effect_track_add: frames / 15 * 1.5 s; decomp39)
    short num_materials;                      // +0x14 records at +0x44 (decomp67; was _unk14)
    short num_desc21c;                        // +0x16 records at +0x2c (decomp64)
    short flags;                              // +0x18 bits 1 and 2 are copied into the instance's bits 7 / 8
                                              //       (vfx_load; meaning not known); bit 0: random yaw at
                                              //       creation, and the objects that follow a character keep
                                              //       their own orientation (decomp64)
    short _pad1a;
    int category;                             // +0x1c copied from the base record's category
    vfx_node_desc *node_descs;                // +0x20 num_nodes descriptors (was int _unk20; decomp64)
    char *light_tracks;                       // +0x24 0x70 bytes per light of an instance (ui_3d_begin; decomp48)
    vfx_emitter_desc *emitters;               // +0x28 num_emitters records
    vfx_desc21c *descs21c;                    // +0x2c num_desc21c records (decomp64)
    vfx_dummy_desc *dummies;                  // +0x30 num_rec2c records (decomp67; was char _unk30[4])
    char *rec4c_table;                        // +0x34 0xa0 bytes per fx_rec4c record (vfx_force_desc)
    vfx_camera_desc *cameras;                 // +0x38 num_cameras records (decomp67)
    vfx_base_tag *tags;                       // +0x3c decomp63: num_tags records
    vfx_material_link *_unk40;                // +0x40 nonzero: the effect can be linked to a character
                                              //       (fx_link_character_obj; decomp64); decomp67: a record
                                              //       whose +0x0c names the material setup_render_state uses
                                              //       (was int)
    vfx_material *materials;                  // +0x44 num_materials records (decomp67)
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
    // decomp64: the first argument is the frame time (a float: fx_update_instance passes its float
    // argument, the creators 0.001f); it was declared int (D4402).
    // decomp67: returns AL (false: no base, disabled base or done; D4700).
    bool process(float frametime, const vector3 *pos, const matrix *orient);   // 0x536fb0 vfx_instance_process
    void stop();                                                     // 0x537230 vfx_instance__stop (decomp64)
    bool is_stopped();                                               // 0x537270 vfx_instance__is_stopped (decomp64)
    void set_stopped(bool stopped);                                  // 0x537280 vfx_instance__set_stopped (decomp67)
    const char *get_node_name(int node);                             // 0x5373a0 (decomp64) decomp67: the node's parent name
    const char *get_tag_name(int tag);                               // 0x537490 (decomp67)
    void bind_node(int node, character_instance *ci, int tag);       // 0x537580 (decomp64)
    bool node_can_bind(int node);                                    // 0x537700 (decomp64)
    // decomp67: (loop, b): clears the stopped and done bits, bit 1 = loop, bit 3 = b, time 0 (bool, D4700).
    void reset_flags(bool a, bool b);                                // 0x537800 (decomp64)
    void get_node_transform(vector3 *pos, matrix *orient, int node); // 0x537840
    // The world transform of a node, the vector and matrix passed by value (spell_probe_ground_point).
    // decomp67: c is a bool (the tags whose parent it is follow; D4700); a and b are only passed down.
    void get_node_world_transform(int node, vector3 pos, matrix orient, int a, int b, bool c); // 0x537a10
    int  find_tag(const char *name);                                 // 0x537f10, -1 (">>> FAILED TO GET TAG %s")
    // decomp67: hides (a) or shows the tag, with b also every tag whose parent it is; no result (D4700).
    void get_tag_transform(int tag, bool a, bool b);                 // 0x537f70 (result unused by spells.cpp)
    float get_time_left();                                           // 0x538510 (decomp37)
    // decomp60 (vfx_instance_create / vfx_instance_free; both in another vsdk\geom unit).
    // decomp67: returns 0 (EAX); the second argument goes to vfx_node_create_light's bool (D4700).
    int  init_nodes(vfx_base *base, bool no_light, bool level);      // 0x536a70 vfx_instance_init_nodes
    void release_nodes();                                            // 0x5369f0 vfx_instance_release_nodes
    // decomp67: inline; their out-of-line copies are COMDATs at the end of the vfx object (built without
    // inline expansion). Defined in effect_mem.h, after effect_get_base and the record types.
    vfx_base *get_base();                                            // 0x538590 vfx_instance__get_base
    fx_rec98 *list18_item(int n);                                    // 0x5385b0 vfx_instance__list18_item
    vfx_light_desc *list1c_item(int n);                              // 0x5385d0 vfx_instance__list1c_item (steps by 2, sic)
    fx_recb0 *list20_item(int n);                                    // 0x5385f0 vfx_instance__list20_item
    vfx_inst21c *list24_item(int n);                                 // 0x538610 vfx_instance__list24_item
    fx_rec2c *list28_item(int n);                                    // 0x538630 vfx_instance__list28_item
    unsigned int id_bits;                                            // +0x00 effect name index << 9 (cast_effect_track_add; decomp39);
                                                                     // bits 0-8 flags: 0x40 in use, 0x80 / 0x100 the base's flag_a / flag_b,
                                                                     // interface_frame clears bit 2 of the UI lights' instance (decomp48)
                                                                     // decomp67: 1 stopped, 2 loop, 4 done, 8 drawn when done,
                                                                     // 0x10 wrapped this frame, 0x20 lights on
    union {
        int _unk04;                                                  // +0x04
        ubyte render_alpha;                                          //       decomp63: the effect object's bits >> 24
                                                                     //       before each draw, 0xff for the sky effects
    };
    int category;                                                    // +0x08 the base record's category (vfx_load)
    union {
        int _unk0c;                                                  // +0x0c
        float elapsed;                                               //       decomp67: seconds; time = elapsed * 15
    };
    float time;                                                      // +0x10 elapsed (fx_frame compares it with 14 / 55.25; decomp64)
                                                                     //       decomp67: the frame
    float length;                                                    // +0x14 longest emitter, seconds
    fx_rec98 *nodes;                                                 // +0x18 list heads, NULL empty
    vfx_light_desc *lights;                                          // +0x1c first light, NULL none (ui_3d_begin; decomp48)
    fx_recb0 *emitters;                                              // +0x20
    union {
        int _unk24;                                                  // +0x24
        vfx_inst21c *insts21c;                                       //       decomp63: the records of the base's descs21c
    };
    fx_rec2c *rec2c_list;                                            // +0x28
    fx_rec4c *rec4c_list;                                            // +0x2c
    vector3 pos;                                                     // +0x30 the base's pos (vfx_load)
    float _unk3c;                                                    // +0x3c the base's +0x54
    vfx_instance *next;                                              // +0x40
    vfx_instance *prev;                                              // +0x44
    // decomp63: (ref, world position, orientation): draws one node or emitter (thiscall, ret 0xc;
    // sum.symbols vfx_instance_render). levelrender_draw_list sets render_alpha first.
    void render_node(vfx_node_ref *ref, const vector3 *pos, const matrix *orient);   // 0x538260
};
// decomp63: the character effect render state around character_instance::render (cdecl, two pushes;
// the first argument receives the effect alpha) and its reset.
void vfx_instance__setup_render_state(ubyte *alpha, vfx_instance *vi);   // 0x538430
void vfx_render_state_reset();                                          // 0x5384f0
// The particle functions of the +0x2c descriptors (cdecl; unit not known): (record, out centre, out
// radius) the bounds of the live particles, (instance record, record, flag, pos, orient, vfx_instance
// +0x0c) one update, (instance record, record) the draw.
void vif_record_get_bounds(vfx_desc21c *rec, vector3 *centre, float *radius);   // 0x533aa0
// decomp67: the last argument is the float vfx_instance::elapsed, passed as its raw dword (D4704).
void vif_emitter_update(vfx_inst21c *em, vfx_desc21c *rec, int flag, const vector3 *pos,
                        const matrix *orient, int unk0c);                   // 0x532f40
void vif_draw_particles(vfx_inst21c *em, vfx_desc21c *rec);               // 0x533620
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
    vector3 world_pos;                        // +0x20 levelrender's light position; decomp63 (D4303): the light
                                              //       position relative to the level object being drawn, in its frame
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
// decomp64: the instance vfx_instance_process works on (cleared by fx_destroy_all).
extern void *vfx_active_instance;                    // 0x2dbfd10
// Inline: its only copy is a COMDAT among the attached-light unit's (0x5203c0, between the track
// samplers and vfx_node__get_scale). (pos, intensity, range far, r, g, b), range near 0.
inline int attached_light_create_from_vfx(const vector3 *pos, float intensity, float range_far,
                                          float r, float g, float b)   // 0x5203c0
{ return attached_light_create(pos, intensity, 0, range_far, r, g, b); }

// The camera shake of the running effects (state_game_frame adds it to the view; decomp42).
void vfx_get_camera(vector3 *pos, quaternion *rot);   // 0x5383e0

// ---- decomp67: the rest of the vfx object's data (0x536950-0x538630, src/decomp/vsdk/unnamed_vfx_0x536950.cpp) ----
struct gr_vertex;
extern gr_vertex gr_vertex_pool[4000];                // 0x2d90f10 (the mesh renderer's output)
extern vector3 vfx_camera_pos;                        // 0x2d90ee8
extern quaternion vfx_camera_orient;                  // 0x2d90ef8
extern ubyte vfx_render_value_b;                      // 0x2dbfd15 the material's curve 44 * 255 (label ours)
extern int vfx_render_blend;                          // 0x2d90f08 the material's flag bits 3-5 (label ours)
extern int vfx_render_flag;                           // 0x2dbfd18 the material's flag bit 6 (label ours)

// decomp65: the render state vfx_instance__setup_render_state sets for the next mesh and
// vfx_render_state_reset clears (vfx object, not written); the character mesh renderers read both.
extern ubyte vfx_render_alpha;                        // 0x2dbfd14 the alpha of the second (environment) pass
extern int vfx_render_bitmap;                         // 0x5a9cfc (name ours) its bitmap, -1 none

#endif
