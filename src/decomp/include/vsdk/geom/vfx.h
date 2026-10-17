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
class vfx_base;                               // below
class vfx_emitter_desc;                       // below (after the key channels; decomp66)
struct vfx_dummy_desc;                        // below (after the key channels; decomp66)
struct vfx_camera_desc;                       // below (after the key channels; decomp66)
class vfx_force_desc;                         // below
struct vfx_force_params;

// decomp66: inline (cdecl); its copy is a COMDAT at the end of the mesh object (0x564d50), which was built
// without inline expansion; the other vsdk\geom objects call it.
inline void ptr_relocate(void *p, char *base)   // 0x564d50 (*p += base unless NULL)
{ if (*(char **)p != NULL) *(char **)p += (int)base; }
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


// decomp64 (the fx unit): the node descriptors (vfx_base +0x20, 0xa8 bytes each) and the +0x2c records (0x21c)
// as fx_create_instance and fx_frame read them. Names ours.
// decomp67: the node descriptors are the meshes (vfx_mesh__* in sum.symbols).
// decomp70: the mesh object (0x562a30-0x564d62, src/decomp/vsdk/unnamed_vfx_mesh_0x562a30.cpp) gives the
// rest of the layout (D5106). The records it points to are declared after the key channels below.
struct vfx_face;
struct vfx_face_uv;
struct vfx_vnormal;
struct vfx_face_uv_anim;
struct vfx_mesh_morph;
struct vfx_mesh_anim;
struct vfx_mesh_vertices;
struct vfx_mesh_sprite;
struct vfx_mesh_xform;
class anim_track;
// vfx_node_desc::mesh_flags (+0x0c; names ours).
#define VFX_MESH_BINDABLE     0x0001  // can be bound to a mesh tag (decomp67)
#define VFX_MESH_SKINNED      0x0002  // anim_track at +0x6c over the rest pose
#define VFX_MESH_X64          0x0004  // vertex positions * 64
#define VFX_MESH_POS_FIXED4   0x0008  // xform positions / 16
#define VFX_MESH_STATIC_VERTS 0x0010  // animated mesh: vertices, not morph targets
#define VFX_MESH_STATIC_UVS   0x0020  // uv animation ignored
// vfx_node_desc::flags (+0x18; decomp70, names ours; 0x40, 0x200, 0x400 are decomp63's).
#define VFX_MESH_SPRITE       0x0001  // a camera-facing sprite (sprite record at +0x60)
#define VFX_MESH_NO_LERP      0x0002  // no blending between frames
#define VFX_MESH_ANIMATED     0x0004  // morph / anim records instead of the xform
#define VFX_MESH_FULLBRIGHT   0x0010  // vertex colours 255, no lighting
#define VFX_MESH_UV_ANIM      0x0100  // face uv animation at +0x58
#define VFX_MESH_LINE         0x0800  // a line quad along line_axis (+0x9c)

// D5107: a POD stand-in for vector3 (a union member cannot have vector3's copy constructor).
struct vfx_vector3_alias {
    float x, y, z;
    operator vector3() const { return vector3(x, y, z); }
};

struct vfx_node_desc {
    void relocate(char *header_base, char *base);   // 0x5645b0 vfx_mesh__relocate (decomp70: the mesh object)
    void load_bitmap(int flags);              // 0x564720 vfx_mesh__load_bitmap (decomp67)
    // decomp70: inline; their copies are COMDATs at the end of the mesh object.
    bool is_billboard() { return (vis_flags & (VFX_MESH_SPRITE | VFX_MESH_LINE)) != 0; }   // 0x564860
    bool frame_bit(int frame);                // 0x564d00 vfx_mesh__frame_bit (inline, below BitArray)
    bool has_flag4() { return (vis_flags >> 2) & 1; }   // 0x564d20 vfx_mesh__has_flag4 (VFX_MESH_ANIMATED)
    const char *name;                         // +0x00 (decomp67; was ubyte _unk00[0x18])
    const char *parent;                       // +0x04
    vfx_base *base;                           // +0x08 decomp70: the header's data pointer (relocated against base)
    union {
        ubyte bind_flags;                     // +0x0c bit 0: can be bound to a mesh tag (decomp67)
        ushort mesh_flags;                    //       decomp70: VFX_MESH_* above, bits 6-15 the frame rate
    };
    ushort num_frames;                        // +0x0e decomp70
    ushort num_verts;                         // +0x10
    ushort num_faces;                         // +0x12 also the face uv and uv animation records
    ushort num_materials;                     // +0x14 entries of materials
    ushort num_vnormals;                      // +0x16
    // decomp63 (D4309): levelrender_effect_apply_vis rewrites the low word only (0x483ff0-0x484003):
    // 0x200 shown by a $vis tag, 0x400 always shown (0x600 = drawn).
    union {
        int flags;                            // +0x18 0x40: a lamp glow (fx_create_instance)
        ushort vis_flags;
    };
    vector3 pos;                              // +0x1c decomp63: the node's position in the effect frame
    float glow_radius;                        // +0x28 (decomp63: also the sort radius of its draw entry)
    float cull_radius;                        // +0x2c decomp63: gr_cull_sphere radius (levelrender_add_effect_entry)
    float start_time;                         // +0x30 decomp70: seconds (the frame is (t / 15 - start) * rate)
    int _unk34;
    vfx_face *faces;                          // +0x38 num_faces
    ubyte *materials;                         // +0x3c indices into base->materials (a face's material picks one)
    vfx_vnormal *vnormals;                    // +0x40 num_vnormals
    // +0x44 the BitArray of the frames the mesh is shown in (frame_bit). D5107: decomp63's glow_pos
    // (+0x48) is not a field of the mesh: lamp_glow_update_effects reads the vfx_base's pos (+0x48 of
    // the base, 0x484690); the alias stays for unnamed_render_scene_0x481680.cpp until it is fixed.
    union {
        struct {
            char *frames_data;                //       BitArray::data
            unsigned int frames_num_bytes;    //       BitArray::num_bytes
            bool frames_owned;                //       BitArray::owned
            ubyte _pad4d[3];
            vfx_mesh_morph *morph;            // +0x50 morph targets (animated meshes)
        };
        struct {
            int _alias44;
            vfx_vector3_alias glow_pos;       // (D5107: not a field)
        };
    };
    vfx_mesh_anim *anim;                      // +0x54 sprite / line animation (animated billboards)
    vfx_face_uv_anim *uv_anims;               // +0x58 num_faces
    vfx_mesh_vertices *vertices;              // +0x5c num_verts
    vfx_mesh_sprite *sprite;                  // +0x60 billboards
    vfx_face_uv *face_uvs;                    // +0x64 num_faces
    vfx_mesh_xform *xform;                    // +0x68
    anim_track *track;                        // +0x6c VFX_MESH_SKINNED
    vector3 rest_pos;                         // +0x70 the rest pose of a skinned mesh
    cquat rest_rot;                           // +0x7c
    vector3 rest_scale;                       // +0x84
    int _unk90;
    char *glow_name;                          // +0x94 reloc_ptr against the header base; NULL none
    int glow_bm;                              // +0x98
    vector3 line_axis;                        // +0x9c VFX_MESH_LINE
};
LAYOUT_CHECK(vfx_node_desc, sizeof(vfx_node_desc) == 0xa8);
// decomp72 (the vif unit 0x5329f0-0x533dae, src/decomp/vsdk/unnamed_vif_0x5329f0.cpp): a vif key curve
// (0x64 bytes; vif_curve__evaluate), the spawn path (0xc4 bytes; vif_emitter_spawn_point) and one particle
// (0x30 bytes, plain floats: the record constructor 0x533c70 builds no member of the two list sentinels).
// Names ours.
struct vif_curve {
    float values[12];                         // +0x00
    float times[12];                          // +0x30
    ubyte count;                              // +0x60 keys; 1 = constant
    ubyte _pad61[3];
};
LAYOUT_CHECK(vif_curve, sizeof(vif_curve) == 0x64);
struct vif_spawn_path {
    vector3 points[12];                       // +0x00
    float times[12];                          // +0x90
    ubyte count;                              // +0xc0 (1 a point, 2 a line, more a spline)
    ubyte _padc1[3];
};
LAYOUT_CHECK(vif_spawn_path, sizeof(vif_spawn_path) == 0xc4);
#define VIF_PARTICLE_ALIVE 0x1
struct vif_particle {
    float x, y, z;                            // +0x00 position
    int flags;                                // +0x0c VIF_PARTICLE_ALIVE
    float vx, vy, vz;                         // +0x10 velocity
    vif_particle *next;                       // +0x1c the record's live or free list
    vif_particle *prev;                       // +0x20
    int seed;                                 // +0x24 rand() at the spawn (not read by the vif unit)
    float age;                                // +0x28 seconds
    float lifetime;                           // +0x2c
};
LAYOUT_CHECK(vif_particle, sizeof(vif_particle) == 0x30);
// decomp72: vis_flags bits the vif unit tests (names ours; 0x80 / 0x100 / 0x200 as above).
#define VIF_ADDITIVE        0x0002            // drawn with gr_mode_particle_additive, else _alpha
#define VIF_SPAWN_BOX       0x0004            // spawned on the emitter's rvec / fvec plane, else a sphere
#define VIF_GRAVITY         0x0008            // vy -= 9.8 * dt
#define VIF_SHOWN_ALONE     0x0010            // updated although not 0x180
#define VIF_FORCE           0x0800            // the force curve pushes the particles
#define VIF_FORCE_ATTRACT   0x1000            //   towards / away from the attract point, else along force_dir
#define VIF_SPAWN_PATH      0x2000            // the origin moves along the spawn path with the effect time
// decomp67: the vif emitter descriptors (vif_desc__* in sum.symbols). decomp72: the fields from the vif unit.
struct vfx_desc21c {
    void relocate(char *header_base);         // 0x533be0 vif_desc__relocate (another unit; decomp67)
    void load_bitmap(int flag);               // 0x533c20 vif_desc__load_bitmap
    const char *name;                         // +0x00 (decomp67; was ubyte _unk00[0x4c])
    const char *parent;                       // +0x04
    vector3 pos;                              // +0x08 (get_node_world_transform measures from it)
    matrix orient;                            // +0x14 (decomp72; was _unk14)
    float size;                               // +0x38 half the drawn size
    float spread_x;                           // +0x3c spawn radius / box half-width (rvec)
    float spread_y;                           // +0x40 box half-depth (fvec)
    float speed;                              // +0x44
    float speed_spread;                       // +0x48 +- random
    float time_a;                             // +0x4c the particle lifetime
    float time_b;                             // +0x50 (time_a + time_b) s (fx_frame kind 16); +- random lifetime
    float size_in;                            // +0x54 age fractions of the size ramp up / down
    float size_out;                           // +0x58
    float alpha_in;                           // +0x5c age fractions of the alpha ramp up / down
    float alpha_out;                          // +0x60
    float alpha;                              // +0x64 alpha scale
    ushort count;                             // +0x68 particles per record
    ushort vis_flags;                         // +0x6a decomp63: 0x80 always shown, 0x100 shown by a $vis tag
                                              //       (0x180 = drawn; the vif_* particle functions take these)
                                              //       decomp67: 0x200 can be bound to a mesh tag; VIF_* above
    int bitmap;                               // +0x6c bm_load handle (relocate clears it)
    const char *bitmap_name;                  // +0x70
    int _unk74;                               // +0x74
    vector3 attract_offset;                   // +0x78 the attract point, from the effect position
    vector3 force_dir;                        // +0x84
    vif_curve force_curve;                    // +0x90 force per second over the effect time
    vif_curve interval_curve;                 // +0xf4 seconds between spawns over the effect time
    vif_spawn_path path;                      // +0x158
};
LAYOUT_CHECK(vfx_desc21c, sizeof(vfx_desc21c) == 0x21c);

// decomp63 (the render-scene unit 0x481680): the base's tags (+0x3c) and the per-instance records of the
// +0x2c descriptors (a list from vfx_instance +0x24), as levelrender_effect_apply_vis /
// levelrender_add_effects / fx_draw_flagged_particles read them. Names ours.
struct vfx_tag_ref {                          // 8 bytes
    // decomp66: inline (COMDAT 0x565450 vfx_group_entry__relocate, after vfx_base_tag::relocate).
    void relocate(char *base) { ptr_relocate(&rec, base); }
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
// decomp72: the vif unit's view (100 records at vif_records 0x2d064e8, init_vif_records constructs them with
// the implicit constructor 0x533c70: the two vectors and the two matrices only).
struct vfx_inst21c {                          // the instance record of a vfx_desc21c
    vector3 pos;                              // +0x00 (decomp67)
    matrix orient;                            // +0x0c
    vector3 emit_pos;                         // +0x30 decomp72: the effect's position and orientation
    matrix emit_orient;                       // +0x3c   (vif_emitter_update's pos / orient)
    float time;                               // +0x60 the effect time of the last update
    float interval;                           // +0x64 the spawn interval of the last update
    ushort flags;                             // +0x68 0x10 updated this frame (fx_draw_flagged_particles)
                                              //       decomp67: 2 placed by get_node_world_transform, 4 bound,
                                              //       8 hidden (get_tag_transform); decomp72: 1 in use,
                                              //       0x20 particles from the heap (level effects)
    short tag;                                // +0x6a the mesh tag it is bound to (decomp67)
    character_instance *ci;                   // +0x6c
    ushort count;                             // +0x70 particles (decomp72)
    ushort num_live;                          // +0x72 live particles (decomp67)
    float spawn_timer;                        // +0x74 decomp72: seconds to the next spawn
    int first;                                // +0x78 index in vif_particles' live half, -1 heap
    vif_particle *particles;                  // +0x7c
    vif_particle live;                        // +0x80 list sentinels (decomp72)
    vif_particle free;                        // +0xb0
    vfx_inst21c *next;                        // +0xe0
    vfx_inst21c *prev;                        // +0xe4 (decomp67)
};
LAYOUT_CHECK(vfx_inst21c, sizeof(vfx_inst21c) == 0xe8);
// decomp67: the vfx object's calls into the vif unit (cdecl).
vfx_inst21c *vif_instantiate_level(vfx_desc21c *desc, bool level);   // 0x5337a0
void vif_record_release(vfx_inst21c *em, vfx_desc21c *desc);       // 0x5339b0
// Inline: the out-of-line copy is 0x5385a0 (sum.symbols vfx_get_word_72), a COMDAT at the end of the vfx
// object (decomp67).
inline int vif_record_num_live(vfx_inst21c *em)   // 0x5385a0
{ return em->num_live; }

// decomp67: the other record kinds of a base (names ours; their methods are in other vsdk\geom units).
struct vfx_tex_layer {                        // 0x14 bytes, two at vfx_material +0x0c
    // decomp66: the material unit (0x564d70-0x56565c, src/decomp/vsdk/unnamed_vfx_material_0x564d70.cpp).
    // The empty constructor and destructor are what vfx_material's constructor passes to 'eh vector
    // constructor iterator' (0x564d70 / 0x564d80, sum.symbols vfx_material_layer__ctor / __dtor).
    vfx_tex_layer() {}                        // 0x564d70
    ~vfx_tex_layer() {}                       // 0x564d80
    // The bitmap of the animation at frame t (from its frame rate, the layer's rate and first frame) or
    // at fraction f of a particle's life; the bitmap itself when it has one frame (ret 4).
    int get_frame_bitmap(float t);            // 0x564d90 vfx_tex_layer__get_frame_bitmap
    int get_frame_bitmap_frac(float f);       // 0x564e40 vfx_tex_layer__get_frame_bitmap_frac
    void relocate(char *header_base);         // 0x565360 vfx_tex_layer__relocate (bitmap 0)
    void load_bitmap(int flags);              // 0x565380 vfx_tex_layer__load_bitmap ("" or "$...": none)
    int bitmap;                               // +0x00 -1 none
    char *name;                               // +0x04 decomp66 (reloc_ptr against the header base)
    int start;                                // +0x08 decomp66: first frame
    float rate;                               // +0x0c
    int mode;                                 // +0x10 0 / 2 wrap, 1 clamp
};
LAYOUT_CHECK(vfx_tex_layer, sizeof(vfx_tex_layer) == 0x14);
struct vfx_material {                         // 0x4c bytes, vfx_base +0x44 (count +0x14)
    // decomp66: the constructor (no caller; bases are loaded) and reset are in the material unit.
    vfx_material();                           // 0x564f20 vfx_material__ctor (EH frame: the layers' destructor)
    void reset();                             // 0x564f80 vfx_material__reset
    void relocate(char *header_base, char *base);   // 0x565280 vfx_material__relocate
    void load_bitmaps(int flags);             // 0x5652e0 vfx_material__load_bitmaps
    // decomp66: the byte curves (n bytes, value / 255) sampled at rate * t / 15 with linear blending,
    // clamped to 0..1; curve 34 only when its mode (bits 0-2) is 1, else 0.
    float eval_curve34(float t);              // 0x564ff0 0..1
    float eval_curve48(float t);              // 0x5651b0 0..1
    float eval_curve44(float t);              // 0x5650e0 0..1
    union {
        ushort flags;                         // +0x00 bits 3-5 blend mode, bit 6 (setup_render_state);
                                              //       decomp66: bit 6 additive particles, bits 7-15 the curve rate
        struct {
            short curve34_mode : 3;           //       decomp66 (D4608): read signed (shl 13 / sar 13)
            unsigned short blend : 3;         //       match23 (D8202): unsigned (reset's and eax, 0xffc7)
            short additive : 1;
            unsigned short rate : 9;
        };
    };
    ushort num34;                             // +0x02 decomp66: curve lengths
    ushort num44;                             // +0x04
    ushort num48;                             // +0x06
    ubyte color[3];                           // +0x08 decomp66: the flat particles' rgb scale (/ 255)
    ubyte _unk0b;
    vfx_tex_layer layers[2];                  // +0x0c
    ubyte *curve34;                           // +0x34 decomp66 (was ubyte _unk34[0x18]: curve pointers, name, bitmap)
    char *name;                               // +0x38 a bitmap name ("" after reset)
    int bitmap;                               // +0x3c
    float _unk40;                             // +0x40 0.6 after reset (no reader in these units)
    ubyte *curve44;                           // +0x44
    ubyte *curve48;                           // +0x48
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
// A run-length coded int16 key channel (0xc bytes). decomp66: the out-of-line members are in the vfx
// channel unit (0x55b440-0x55bd77, src/decomp/vsdk/unnamed_vfx_channel_0x55b440.cpp); relocate is
// inline (its COMDAT 0x5202b0 is in the attached-light unit: the AND 0x8000 clears frame and done).
// The data: a byte with bit 0 set starts a run (count byte, then the value: one byte when `bytes`, else
// two), with bit 0 clear a single two-byte key (D4602).
class vfx_channel {
public:
    vfx_channel();                            // 0x55bbc0 vfx_channel__ctor (no caller)
    void clear();                             // 0x55bbb0 vfx_channel__clear (the three words zeroed)
    // The value at a frame (base_value + key); 4000 once the data is found broken (done set).
    short eval(int frame);                    // 0x55bc00 vfx_channel__eval (decomp66, D4602: was int)
    void relocate(char *base)                 // 0x5202b0 vfx_track__relocate (thiscall, ret 4)
    { frame = 0; done = 0; cursor = 0; ptr_relocate(this, base); }
    char *data;                               // +0x00
    ushort size;                              // +0x04 bytes of data
    short base_value;                         // +0x06
    ushort cursor;                            // +0x08 cached key offset, reset by relocate
    ushort frame : 14;                        // +0x0a cached key frame, reset by relocate
    ushort done : 1;                          //       bit 14: bad data, eval returns 4000
    ushort bytes : 1;                         //       bit 15: run values are one byte
};
LAYOUT_CHECK(vfx_channel, sizeof(vfx_channel) == 0xc);

// Three channels: x +0, y +0xc, z +0x18 (0x24 bytes).
class vfx_vec_channel {
public:
    void relocate(char *base);                // 0x55bb50 vfx_vec_channel__relocate
    // The value at a frame, 8.8 fixed (hidden result, ret 4; the z channel first).
    vector3 eval(int frame);                  // 0x55b9e0 vfx_vec_channel__eval (decomp66)
    // The position at a frame (hidden result, ret 8; COMDAT 0x520310). The z channel is evaluated
    // first (the constructor's arguments are pushed right to left).
    // decomp66: the same in 4.12 (inline; COMDAT 0x565610 vfx_vec_channel__eval_fixed12, the material unit).
    vector3 eval_fixed12(int frame)
    { return vector3(fixed12_to_float(x.eval(frame)), fixed12_to_float(y.eval(frame)),
                     fixed12_to_float(z.eval(frame))); }
    vector3 sample(int frame)                 // 0x520310 vfx_vector_track__sample
    { return vector3(fixed4_to_float((short)x.eval(frame)), fixed4_to_float((short)y.eval(frame)),
                     fixed4_to_float((short)z.eval(frame))); }
    vfx_channel x, y, z;
};
LAYOUT_CHECK(vfx_vec_channel, sizeof(vfx_vec_channel) == 0x24);

// Four channels of a cquat (0x30 bytes; decomp66).
class vfx_quat_channel {
public:
    void relocate(char *base);                // 0x55bb80 vfx_quat_channel__relocate
    cquat eval(int frame);                    // 0x55ba50 vfx_quat_channel__eval (hidden result, the w channel first)
    vfx_channel x, y, z, w;
};
LAYOUT_CHECK(vfx_quat_channel, sizeof(vfx_quat_channel) == 0x30);

// A bit array (0xc bytes; at the light track's +0x64, one bit per frame). decomp66: the out-of-line
// members are in the BitArray unit (0x55bd80-0x55bf88, src/decomp/vsdk/unnamed_bitarray_0x55bd80.cpp);
// the inline relocate is the COMDAT 0x5202d0 (sum.symbols: vfx_ptr__relocate).
class NoRlsAllocPool;
class BitArray {
public:
    BitArray();                               // 0x55bd80 BitArray__ctor (data = NULL)
    ~BitArray();                              // 0x55bd90 BitArray__dtor (free)
    // (bits, owned): operator new of the byte count; -1 when that fails.
    int alloc(int bits, bool owned);          // 0x55bda0
    void attach(void *buf, int bits);         // 0x55bde0 not owned
    int alloc_from_pool(NoRlsAllocPool *pool, int bits);   // 0x55be00 -1 when the pool is full
    void fill(bool on);                       // 0x55be40 every byte 0 or 0xff
    int get(int bit);                         // 0x55be80 BitArray__get (1 when set)
    void set(int bit, int on);                // 0x55beb0
    void free();                              // 0x55bef0 deletes an owned buffer
    // (bits, value): the whole bytes are compared with 0 whatever value is, and the tail range is
    // computed wrongly (start + bits, mod 8), so the tail is never tested (NOTES.md).
    bool all_equal(unsigned int bits, bool value);   // 0x55bf20
    void relocate(char *base)                 // 0x5202d0 vfx_ptr__relocate (thiscall, ret 4)
    { ptr_relocate(this, base); }
    char *data;                               // +0x00
    unsigned int num_bytes;                   // +0x04
    bool owned;                               // +0x08
    ubyte _pad09[3];
};
LAYOUT_CHECK(BitArray, sizeof(BitArray) == 0xc);

// The header of an effect file record (0x10 bytes): two name / link fields, a pointer and the frame count.
// decomp66 (D4601): the word at +0xc is read two ways: the light tracks take a 15-bit count above bit 0
// of the low half (SHR 1 of the ushort), the forces an arithmetic SAR 8 of the whole dword (frame_word;
// its low byte is the force kind). Both views are kept as an anonymous union.
class vfx_node_header {
public:
    void relocate(char *header_base, char *base);   // 0x55bbd0 vfx_node_header__relocate
    int name;                                 // +0x00 (decomp67: the name; reloc_ptr)
    int link;                                 // +0x04 (decomp67: the parent name)
    char *data;                               // +0x08
    union {
        struct {
            ushort _bit0 : 1;                 // +0x0c
            ushort num_frames : 15;           //       frames of the light's tracks (sample: SHR 1)
            short _unk0e;
        };
        int frame_word;                       // +0x0c frames << 8 for the forces (vfx_force_desc::num_frames)
    };
};
LAYOUT_CHECK(vfx_node_header, sizeof(vfx_node_header) == 0x10);

// A node of an effect file (0x64 bytes): the header, the position and the orientation channels
// (decomp66; class name from sum.symbols vfx_node__get_pos).
class vfx_node {
public:
    vector3 get_pos(int frame);               // 0x55b9c0 vfx_node__get_pos (pos.eval)
    cquat get_orient(int frame);              // 0x55ba30 vfx_node__get_orient (orient.eval)
    int num_frames() const { return header.frame_word >> 8; }
    vfx_node_header header;                   // +0x00
    vfx_vec_channel pos;                      // +0x10 8.8 fixed
    vfx_quat_channel orient;                  // +0x34
};
LAYOUT_CHECK(vfx_node, sizeof(vfx_node) == 0x64);

// ---- decomp70: the mesh records (vfx_node_desc's pointers; the mesh object 0x562a30-0x564d62) -----
// Every accessor is inline; the copies are the COMDATs at the end of the mesh object (0x564770-0x564d62),
// which was built without inline expansion. Type names ours (sum.symbols names the accessors).
inline bool vfx_node_desc::frame_bit(int frame)
{ return ((BitArray *)&frames_data)->get(frame) != 0; }
// (f): (int)(f + 0.5) (cdecl; vsdk\math in sum.symbols).
inline int float_round_to_int(float f)        // 0x5648b0
{ return (int)(f + 0.5f); }

// One triangle (0x1c bytes, vfx_node_desc +0x38).
struct vfx_face {
    // The face normal from the three bytes: each * (1 / 127.5) - 1, read as signed bit fields (sar), so
    // components above 0 come out wrong (D5108; hidden result).
    vector3 get_normal()                      // 0x564a20 vfx_unpack_vector_s8
    { return vector3(nx * (1.0f / 127.5f) - 1.0f, ny * (1.0f / 127.5f) - 1.0f, nz * (1.0f / 127.5f) - 1.0f); }
    int material : 8;                         // +0x00 index into vfx_node_desc::materials; < 0 none
    int nx : 8;
    int ny : 8;
    int nz : 8;
    ushort v[3];                              // +0x04 vertex indices (vfx_mesh_verts / gr_vertex_pool)
    ubyte _unk0a[0x0c];
    ushort vn[3];                             // +0x16 vertex normal records (vfx_node_desc::vnormals)
};
LAYOUT_CHECK(vfx_face, sizeof(vfx_face) == 0x1c);

// The texture coordinates of one face (0xc bytes, 4.12): vfx_node_desc +0x64 and vfx_mesh_face_uvs.
struct vfx_face_uv {
    float get_u(int i) { return fixed12_to_float(u[i]); }            // 0x564870 vfx_face__get_u
    float get_v(int i) { return fixed12_to_float(v[i]); }            // 0x564890 vfx_face__get_v
    void set_u(int i, float f) { u[i] = (short)float_to_fixed12(f); }   // 0x564aa0 vfx_face__set_u
    void set_v(int i, float f) { v[i] = (short)float_to_fixed12(f); }   // 0x564ac0 vfx_face__set_v
    short u[3];
    short v[3];
};
LAYOUT_CHECK(vfx_face_uv, sizeof(vfx_face_uv) == 0xc);

// A shared vertex normal and its lit colour (0xc bytes, vfx_node_desc +0x40): the normal is the mean of
// its faces' normals, the colour lights_shade_vertices' (vfx_mesh_render, once per frame: done).
struct vfx_vnormal {
    vector3 unpack_normal()                   // 0x5648c0 vfx_unpack_normal (hidden result)
    { return vector3(nx * (1.0f / 127.5f) - 1.0f, ny * (1.0f / 127.5f) - 1.0f, nz * (1.0f / 127.5f) - 1.0f); }
    void pack_normal(const vector3 *n)        // 0x564950 vfx_pack_normal (each (c + 1) * 127.5 in 0..255)
    {
        nx = float_max(0.0f, float_min(255.0f, (n->x + 1.0f) * 127.5f));
        ny = float_max(0.0f, float_min(255.0f, (n->y + 1.0f) * 127.5f));
        nz = (ubyte)float_max(0.0f, float_min(255.0f, (n->z + 1.0f) * 127.5f));
    }
    void relocate(char *base) { ptr_relocate(&faces, base); }   // 0x564d30 vfx_ptr4_relocate
    unsigned int done : 1;                    // +0x00 lit this frame
    unsigned int vertex : 11;
    unsigned int nx : 8;
    unsigned int ny : 8;
    unsigned int count : 4;                   //       entries of faces
    ushort *faces;                            // +0x04 the faces sharing the vertex
    ubyte r, g, b;                            // +0x08
    ubyte nz;                                 // +0x0b
};
LAYOUT_CHECK(vfx_vnormal, sizeof(vfx_vnormal) == 0xc);

// The uv animation of one face (0x48 bytes, vfx_node_desc +0x58): three u and three v channels, 4.12.
struct vfx_face_uv_anim {
    void relocate(char *base)                 // 0x564810 vfx_face_uv_anim__relocate
    { for (int i = 0; i < 3; i++) { u[i].relocate(base); v[i].relocate(base); } }
    float get_u(int i, int frame) { return fixed12_to_float(u[i].eval(frame)); }   // 0x564cc0
    float get_v(int i, int frame) { return fixed12_to_float(v[i].eval(frame)); }   // 0x564ce0
    vfx_channel u[3];
    vfx_channel v[3];
};
LAYOUT_CHECK(vfx_face_uv_anim, sizeof(vfx_face_uv_anim) == 0x48);

// Morph targets (vfx_node_desc +0x50): one 8.8 position channel per vertex, and the origin.
struct vfx_mesh_morph {
    void relocate(int count, char *base)      // 0x5647a0 vfx_mesh_morph__relocate
    {
        ptr_relocate(&channels, base);
        for (int i = 0; i < count; i++)
            channels[i].relocate(base);
    }
    vector3 get_pos(int vertex, int frame) { return channels[vertex].eval(frame); }   // 0x564b80
    vfx_vec_channel *channels;                // +0x00
    vector3 origin;                           // +0x04
};

// The animation of a billboard (0x3c bytes, vfx_node_desc +0x54): width, height (8.8) and position.
struct vfx_mesh_anim {
    void relocate(char *base)                 // 0x5647e0 vfx_mesh_anim__relocate
    { a.relocate(base); b.relocate(base); vec.relocate(base); }
    float get_a(int frame) { return fixed8_to_float(a.eval(frame)); }   // 0x564c00
    float get_b(int frame) { return fixed8_to_float(b.eval(frame)); }   // 0x564c20
    vector3 get_vec(int frame) { return vec.eval(frame); }              // 0x564c40
    vfx_channel a;                            // +0x00 width
    vfx_channel b;                            // +0x0c height
    vfx_vec_channel vec;                      // +0x18 position
};
LAYOUT_CHECK(vfx_mesh_anim, sizeof(vfx_mesh_anim) == 0x3c);

// The static vertices (vfx_node_desc +0x5c): x, y, z 8.8 per vertex, and the origin.
struct vfx_mesh_vertices {
    void relocate(char *base) { ptr_relocate(&data, base); }   // 0x564840 vfx_ptr0_relocate
    vector3 get_vertex(int i)                 // 0x564bb0 vfx_mesh__get_vertex (hidden result)
    { return vector3(fixed8_to_float(data[i * 3]), fixed8_to_float(data[i * 3 + 1]), fixed8_to_float(data[i * 3 + 2])); }
    short *data;                              // +0x00
    vector3 origin;                           // +0x04
};

// The size and position of a billboard (vfx_node_desc +0x60), 8.8. D5109: get_w / get_h are the code of
// fx_rec28::get_age / get_lifetime (effect_mem.h); the linker kept one COMDAT for each pair, in this
// object, under the particle names in sum.symbols.
struct vfx_mesh_sprite {
    float get_w() { return fixed8_to_float(w); }   // folded with fx_rec28::get_age (D5109)
    float get_h() { return fixed8_to_float(h); }   // folded with fx_rec28::get_lifetime (D5109)
    vector3 get_pos()                         // 0x564c80 vfx_get_fixed8_vector_4
    { return vector3(fixed8_to_float(x), fixed8_to_float(y), fixed8_to_float(z)); }
    short w, h;                               // +0x00
    short x, y, z;                            // +0x04
};

// The transform of a mesh (0x84 bytes, vfx_node_desc +0x68).
struct vfx_mesh_xform {
    void relocate(char *base)                 // 0x564770 vfx_mesh_xform__relocate
    { pos.relocate(base); scale.relocate(base); rot.relocate(base); }
    // (fixed4, frame): + origin; with fixed4 (VFX_MESH_POS_FIXED4) the channel value / 16 (hidden result).
    vector3 get_pos(int fixed4, int frame)    // 0x564ae0 (fixed4 tested as a dword)
    { return (fixed4 ? pos.eval(frame) * 0.0625f : pos.eval(frame)) + origin; }
    vector3 get_scale(int frame) { return scale.eval(frame); }   // 0x564b40
    cquat get_rot(int frame) { return rot.eval(frame); }         // 0x564b60
    vector3 origin;                           // +0x00
    vfx_vec_channel pos;                      // +0x0c
    vfx_vec_channel scale;                    // +0x30
    vfx_quat_channel rot;                     // +0x54
};
LAYOUT_CHECK(vfx_mesh_xform, sizeof(vfx_mesh_xform) == 0x84);

// decomp70: the mesh object's scratch arrays (defined with effect_mem.cpp's .bss: 0x272b900 in
// effect_mem.cpp; 0x2c98a90-0x2c9f1af lies in its .bss but only the mesh code uses it, D5110).
extern vector3 vfx_mesh_verts[1820];          // 0x272b900 this frame's vertices (update_frame)
extern vfx_face_uv vfx_mesh_face_uvs[2200];   // 0x2c98a90 this frame's face uvs (update_frame)


// A force of an effect file (0xa0 bytes, the base's table at +0x34; decomp66). get_channel64 is the
// strength (int16 * 2000), the other four 8.8 fixed. decomp66 (D4606): decomp67's vfx_force_desc stub
// (struct, relocate only) and decomp66's vfx_force are one class; the name is decomp67's. The low byte of
// header.frame_word is the kind (1: a point force, else a plane; the emitter unit).
class vfx_force_desc : public vfx_node {
public:
    void relocate(char *header_base, char *base);   // 0x55b960 vfx_force__relocate
    float get_channel64(int frame);           // 0x55baa0
    float get_channel70(int frame);           // 0x55bad0
    float get_channel7c(int frame);           // 0x55baf0
    float get_channel88(int frame);           // 0x55bb10
    float get_channel94(int frame);           // 0x55bb30
    vfx_channel ch64;                         // +0x64
    vfx_channel ch70;                         // +0x70
    vfx_channel ch7c;                         // +0x7c
    vfx_channel ch88;                         // +0x88
    vfx_channel ch94;                         // +0x94
};
LAYOUT_CHECK(vfx_force_desc, sizeof(vfx_force_desc) == 0xa0);

// A force evaluated at a time (0x2c bytes; fx_rec4c::get_transform's output, the emitter particle code's
// input; names ours).
struct vfx_force_params {
    vector3 pos;                              // +0x00
    vector3 dir;                              // +0x0c the force's up axis (0, 1, 0) turned by its rotation
    float strength;                           // +0x18 ch64
    float v1c, v20, v24, v28;                 // +0x1c ch70, ch7c, ch88, ch94; decomp66 (the emitter
                                              //       unit): falloff (exp(-v1c d)), turbulence amount,
                                              //       turbulence speed (* time * 48), turbulence scale
};
LAYOUT_CHECK(vfx_force_params, sizeof(vfx_force_params) == 0x2c);

// decomp67: the other record kinds of a base. decomp66: the dummy's channels (moved after the key
// channels); the dummy unit (0x55fc70-0x55fefa) reads the descriptor as a vfx_node (D4606).
struct vfx_dummy_desc {                       // 0x64 bytes, vfx_base +0x30 (count +0x0a)
    void relocate(char *header_base, char *base);   // 0x55fed0 vfx_dummy__relocate
    // decomp66: the light track's sampler; the dummy unit calls the COMDAT 0x5202f0 (identical code
    // folded, as fx_rec_2vec__ctor).
    vector3 get_position(int frame)           // 0x5202f0 (folded)
    { return pos.sample(frame); }
    const char *name;                         // +0x00
    const char *parent;                       // +0x04
    ubyte _unk08[0x04];
    union {                                   // +0x0c
        ushort flags;                         //       0x8000: can be bound; decomp66: bits 0-14 the frame count
        struct {                              //       match17 (D7600): node_can_bind reads bit 15 as a 16-bit
            ushort num_frames : 15;           //       bitfield (`mov ax, [..]; shr ax, 15`)
            ushort can_bind : 1;
        };
    };
    ubyte _unk0e[0x10 - 0x0e];
    vfx_vec_channel pos;                      // +0x10 decomp66
    vfx_quat_channel orient;                  // +0x34
};
LAYOUT_CHECK(vfx_dummy_desc, sizeof(vfx_dummy_desc) == 0x64);
// decomp67: a camera of a base. decomp66: its fields (moved after the key channels); the descriptor is
// a vfx_node for get_orient (D4606).
struct vfx_camera_desc {                      // 0x64 bytes, vfx_base +0x38 (count +0x0e)
    void relocate(char *header_base, char *base);   // 0x5655c0 vfx_camera__relocate
    // false outside its frame range, else the camera at frame t.
    bool get_transform(float t, vector3 *pos, quaternion *rot);   // 0x565470 vfx_camera__get_transform
    // decomp66: inline (COMDAT 0x5655f0, at the end of the material unit; hidden result, ret 8).
    vector3 get_pos(int frame) { return pos.eval_fixed12(frame); }
    const char *name;                         // +0x00
    const char *parent;                       // +0x04
    ubyte _unk08[0x04];
    short start_frame;                        // +0x0c decomp66
    short end_frame;                          // +0x0e
    vfx_vec_channel pos;                      // +0x10 4.12 fixed
    vfx_quat_channel orient;                  // +0x34
};
LAYOUT_CHECK(vfx_camera_desc, sizeof(vfx_camera_desc) == 0x64);

// One emitter record of a base (0xe8 bytes, the array at vfx_base +0x28; name ours).
// decomp66: moved after the key channels (its channels at +0x4c..+0xe7); the field names from the
// emitter unit (0x55ff00-0x562a2b, src/decomp/vsdk/unnamed_vfx_emitter_0x55ff00.cpp; ours). The
// accessors are inline: their copies are COMDATs at the end of the emitter object (0x5622b0-0x562a2b),
// which was built without inline expansion.
class vfx_emitter_desc {
public:
    // (rate + 1) * frames / 4800 (thiscall; a COMDAT after effect_mem.cpp's record constructors).
    float get_length_seconds()                // 0x51e020 vfx_get_length_seconds
    { return (rate + 1.0f) * frames * (1.0f / 4800.0f); }
    // decomp66: the header part is vfx_node_header's layout (+0x00..+0x0b; the emitter's +0x0c is links[0]).
    void relocate(char *header_base, char *base);   // 0x561190 vfx_emitter__relocate
    float get_rate(int frame)                 // 0x5622b0 vfx_emitter__get_rate (particles per frame / 320)
    { return fixed12_to_float(rate_ch.eval(frame)); }
    vector3 get_offset(int frame)             // 0x562460 vfx_emitter__get_offset (hidden result)
    { return offset.sample(frame); }
    cquat get_orient(int frame)               // 0x562480 vfx_emitter__get_orient (hidden result)
    { return orient.eval(frame); }
    short eval_channel_a0(int frame) { return ch_a0.eval(frame); }   // 0x5624a0 width
    short eval_channel_ac(int frame) { return ch_ac.eval(frame); }   // 0x5624c0 depth
    short eval_channel_b8(int frame) { return ch_b8.eval(frame); }   // 0x5624e0 size
    short eval_channel_c4(int frame) { return ch_c4.eval(frame); }   // 0x562500 speed
    short eval_channel_d0(int frame) { return ch_d0.eval(frame); }   // 0x562520 speed spread
    float get_fade_in() { return size_fade_in * (1.0f / 255.0f); }   // 0x5627d0 vfx_emitter__get_fade_in
    float get_fade_out() { return size_fade_out * (1.0f / 255.0f); } // 0x5627f0 vfx_emitter__get_fade_out
    float get_fixed8_2e() { return fixed8_to_float(alpha_fade_in); } // 0x562810
    float get_fixed8_30() { return fixed8_to_float(alpha_fade_out); }// 0x562820
    float get_fixed8_32() { return fixed8_to_float(stretch); }       // 0x562a20
    const char *name;                         // +0x00 (decomp67; was char _unk00[0x0c])
    const char *parent;                       // +0x04
    vfx_base *base;                           // +0x08 decomp66: the header's data pointer (relocated against
                                              //       base); the materials are read through it (was _unk08)
    char *links[3];                           // +0x0c records of the base's 0xa0-byte table (+0x34, vfx_force_desc), or NULL
    unsigned int num_links : 31;              // +0x18
    unsigned int _bit31 : 1;                  //       decomp67: the emitter can be bound to a mesh tag
    // decomp67 (D4702): a signed 16-bit bit field (shl / sar 16 in vfx_instance_create, decode_tag and
    // get_num_tags; was short num_particles, short _pad1e); also the emitter's sub-tag count.
    int num_particles : 16;                   // +0x1c particle records (fx_rec28) per instance
    int start_frame : 16;                     //       decomp66: no emission before it (was _bits1c_16)
    int frames;                               // +0x20 particle lifetime in 1/4800 s
    float rate;                               // +0x24 lifetime spread (frames * rate either way)
    // decomp63 (levelrender_add_effects / levelrender_effect_apply_vis; names ours). decomp66: the low
    // bits as the emitter unit reads them (D4605).
    union {
        unsigned int vis_flags;               // +0x28 0x400000 shown by a $vis tag, 0x800000 always shown
        struct {
            int shape : 2;                    //       0 box (falls), 1 sphere; others emit nothing
            int material : 14;                //       vfx_base::materials index
            unsigned int white : 1;           //       0x10000 colour 255 (and additive in the 3D path)
            unsigned int gravity : 1;         //       0x20000 9.8 down
            unsigned int life_anim : 1;       //       0x40000 bitmap frames over the lifetime
            unsigned int add_velocity : 1;    //       0x80000 the emitter's velocity is added
            unsigned int random_rot : 1;      //       0x100000 random start angle
            unsigned int _bit21 : 1;
            unsigned int vis_tag : 1;         //       0x400000
            unsigned int always_shown : 1;    //       0x800000
            unsigned int flat : 1;            //       0x1000000 drawn as stretched triangles (gr_poly)
            unsigned int _bits25 : 7;
        };
    };
    ubyte size_fade_in;                       // +0x2c fraction of the life (/ 255) the size grows
    ubyte size_fade_out;                      // +0x2d from this fraction the size shrinks
    short alpha_fade_in;                      // +0x2e 8.8 fraction, alpha grows
    short alpha_fade_out;                     // +0x30 8.8 fraction, alpha falls from here
    short stretch;                            // +0x32 8.8 (flat particles: length along the velocity)
    char _unk34[0x38 - 0x34];
    int _bits38 : 16;                         // +0x38
    int end_frame : 16;                       //       no emission after it; the frame count of a loop
    vector3 centre;                           // +0x3c bounding sphere (effect frame)
    float radius;                             // +0x48
    vfx_vec_channel offset;                   // +0x4c emission point
    vfx_quat_channel orient;                  // +0x70
    vfx_channel ch_a0;                        // +0xa0 width (box) / radius (sphere), 4.12 fixed
    vfx_channel ch_ac;                        // +0xac depth (box)
    vfx_channel ch_b8;                        // +0xb8 particle size
    vfx_channel ch_c4;                        // +0xc4 speed
    vfx_channel ch_d0;                        // +0xd0 speed spread
    vfx_channel rate_ch;                      // +0xdc emission rate
};
LAYOUT_CHECK(vfx_emitter_desc, sizeof(vfx_emitter_desc) == 0xe8);

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
// decomp72: the rest of the vif unit (cdecl; names from sum.symbols, the spawn-path / curve types ours).
void vif_compact_particles();                                                 // 0x532a20
void vif_emitter_spawn_point(vector3 *out, const vif_spawn_path *path, float t);   // 0x532cb0
float vif_min_nonzero_float(const vif_curve *c);                              // 0x532e30 (no caller)
void vif_stub_ret_533c60();                                                   // 0x533c60 (bare ret, no caller)
// (curve, time, reciprocal): the value at the time, linear between two keys; with reciprocal set the
// reciprocals are interpolated (with a wrong fraction, see the unit's NOTES); 0 between two keys when
// either is 0.
float vif_curve__evaluate(const vif_curve *c, float t, bool reciprocal);     // 0x533ca0
#define VIF_NUM_RECORDS   100
#define VIF_MAX_PARTICLES 3000
extern vif_particle vif_particles[2][VIF_MAX_PARTICLES];   // 0x2cbffe0 two halves (vif_compact_particles)
extern vif_particle *vif_particle_top;        // 0x5a99d8 the live half
extern int vif_data_top;                      // 0x2d064e0 bytes of heap particles (level effects)
extern vfx_inst21c *vif_record_list;          // 0x2d064e4 the free records (a ring)
extern vfx_inst21c vif_records[VIF_NUM_RECORDS];   // 0x2d064e8
extern int vif_particle_half;                 // 0x2d0bf8c
extern int vif_particle_used;                 // 0x2d0bf90 particles used in the live half
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
    char type;                                // +0x00 -1 free slot (signed: `or cl, -1`, D7202), 0 point (attached_light_create), 1 / 4 (create_b)
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
